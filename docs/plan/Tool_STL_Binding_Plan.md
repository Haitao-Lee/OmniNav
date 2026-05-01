# Plan：Tool-STL 绑定功能

## 需求描述

增加一个功能：让某个 Tool（ROM）和某个 STL 绑定，即 Tool 怎么动，对应的 STL 在三维世界中也这样动。

**典型场景**：有一个 Tool 固定在患者身上，当患者被移动时，通过该 Tool 的位姿变化，自动驱动与之绑定的所有 STL 模型（如肿瘤、血管等）在 3D 视图中同步运动。

---

## 现状分析

### 已有基础

| 组件 | 状态 | 位置 |
|------|------|------|
| Mesh 表格第 4 列 Tool ComboBox | UI 已创建，但**无信号连接** | `DataManager.cpp:1065-1088` |
| Landmark 表格第 6 列 Tool ComboBox | 已实现同 pointset 联动同步 | `DataManager.cpp:1423-1454` |
| Mesh 跟随逻辑（Part 2） | **已注释** | `OmniNav.cpp:889-915` |

### 缺失部分

- Mesh 的 Tool ComboBox 选择变化时没有回调
- `onUpdateNavigationInfo2DataManager()` 中 Mesh 跟随代码被注释
- 没有"绑定时记录初始位姿"的机制

---

## 核心设计问题：绑定语义

### 方式 A：绝对跟随（简单）

```cpp
meshActor->SetUserMatrix(toolFinalMatrix);
```

STL 直接跳到 Tool 当前位置。适用于 Tool 在 CT 扫描时就固定在身上、配准后 Tool 的 `finalMatrix` 已经表达"从追踪坐标系到患者坐标系"变换的场景。

### 方式 B：增量跟随（推荐）

```cpp
deltaMatrix = toolCurrentMatrix * toolInitialMatrix.inverse();
newMatrix = deltaMatrix * meshInitialMatrix;
meshActor->SetUserMatrix(newMatrix);
```

绑定时记录 Tool 的初始位姿，之后只应用相对变化量。适用于 Tool 在 CT 扫描之后才固定、需要保持 STL 原始位置不变、只叠加 Tool 运动增量的场景。

**建议采用方式 B**，因为实际临床中 Tool 通常在扫描后才固定到患者身上。

---

## 实现方案

### 修改点 1：`SRC/Item/Model3D.h` — 增加绑定状态字段

新增成员：

```cpp
std::string m_boundToolName;                              // 绑定的 Tool 名称，空=未绑定
vtkSmartPointer<vtkMatrix4x4> m_toolInitialMatrix;        // 绑定时 Tool 的位姿快照
vtkSmartPointer<vtkMatrix4x4> m_meshInitialMatrix;        // 绑定时 Mesh 的 UserMatrix
```

新增方法：

```cpp
void bindTool(const std::string& toolName, vtkMatrix4x4* toolCurrentMatrix);
void unbindTool();
bool isBound() const;
const std::string& getBoundToolName() const;
void applyToolTransform(vtkMatrix4x4* toolCurrentMatrix);  // 每帧调用
```

### 修改点 2：`SRC/Item/Model3D.cpp` — 实现绑定逻辑

```cpp
void Model3D::bindTool(const std::string& toolName, vtkMatrix4x4* toolCurrentMatrix) {
    m_boundToolName = toolName;
    m_toolInitialMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    m_toolInitialMatrix->DeepCopy(toolCurrentMatrix);
    m_meshInitialMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    if (m_actor->GetUserMatrix()) {
        m_meshInitialMatrix->DeepCopy(m_actor->GetUserMatrix());
    } else {
        m_meshInitialMatrix->Identity();
    }
}

void Model3D::unbindTool() {
    // 恢复到绑定前的位置
    if (m_actor && m_meshInitialMatrix) {
        m_actor->SetUserMatrix(m_meshInitialMatrix);
    }
    m_boundToolName.clear();
    m_toolInitialMatrix = nullptr;
    m_meshInitialMatrix = nullptr;
}

void Model3D::applyToolTransform(vtkMatrix4x4* toolCurrentMatrix) {
    if (m_boundToolName.empty() || !toolCurrentMatrix) return;

    // delta = current * initial.inverse()
    vtkSmartPointer<vtkMatrix4x4> toolInitialInv = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Invert(m_toolInitialMatrix, toolInitialInv);

    vtkSmartPointer<vtkMatrix4x4> delta = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4(toolCurrentMatrix, toolInitialInv, delta);

    // result = delta * meshInitial
    vtkSmartPointer<vtkMatrix4x4> result = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4(delta, m_meshInitialMatrix, result);

    m_actor->SetUserMatrix(result);
}
```

### 修改点 3：`SRC/Modules/DataManager/DataManager.cpp` — 连接 ComboBox 信号

在 `add3Dmodel()` 函数中（约 1088 行之后），为 `toolCbb` 连接信号：

```cpp
connect(toolCbb, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    Model3D* mesh = m_meshes[row];
    if (index == 0) {
        mesh->unbindTool();
    } else {
        QString toolName = toolCbb->itemText(index);
        if (m_toolCache.contains(toolName)) {
            Tool* tool = m_toolCache[toolName];
            mesh->bindTool(toolName.toStdString(), tool->getFinalMatrix());
        }
    }
});
```

同时需要在 Tool 列表变化时同步更新 Mesh 表格第 4 列的 ComboBox 选项（参考 Landmark 表格的同步逻辑）。

### 修改点 4：`SRC/OmniNav.cpp` — 启用 Mesh 跟随逻辑

取消注释 `onUpdateNavigationInfo2DataManager()` 中的 Part 2（约 889-915 行），改用新 API：

```cpp
// Part 2: Update bound meshes
for (int i = 0; i < meshTable->rowCount(); ++i) {
    if (i >= meshes.size()) break;
    Model3D* mesh = meshes[i];
    if (!mesh->isBound()) continue;

    QString boundToolName = QString::fromStdString(mesh->getBoundToolName());
    if (toolCache.contains(boundToolName)) {
        Tool* tool = toolCache[boundToolName];
        vtkMatrix4x4* toolMat = tool->getFinalMatrix();
        if (toolMat) {
            mesh->applyToolTransform(toolMat);
        }
    }
}
```

### 修改点 5：`SRC/OmniNav.cpp` — 同步 Mesh ComboBox 选项

在 Tool 列表变化时，遍历 `mesh_tw` 每一行，同步第 4 列 ComboBox 的选项列表（与 Landmark 表格同步逻辑一致）。

---

## 时序图

```
【绑定阶段】
用户在 DataManager UI → Mesh 表格第 4 列选择 Tool
    → currentIndexChanged 信号触发
    → mesh->bindTool(toolName, toolCurrentMatrix)
        → 记录 toolInitialMatrix（快照）
        → 记录 meshInitialMatrix（快照）

【运行阶段 - 每帧 60Hz】
NDI 设备 → TrackedTool 数据
    → OpticalNavigation → signalAllToolsUpdated
    → OmniNav::onUpdateNavigationInfo2DataManager()
        → 遍历 meshes
        → mesh->applyToolTransform(toolFinalMatrix)
            → deltaMatrix = toolCurrent × toolInitial⁻¹
            → newMatrix = deltaMatrix × meshInitial
            → actor->SetUserMatrix(newMatrix)
            → STL 在 3D 视图中跟随 Tool 运动
```

---

## 文件改动汇总

| 文件 | 改动量 | 内容 |
|------|--------|------|
| `Item/Model3D.h` | ~15 行 | 新增绑定字段和方法声明 |
| `Item/Model3D.cpp` | ~30 行 | 实现 bind/unbind/applyToolTransform |
| `Modules/DataManager/DataManager.cpp` | ~25 行 | 连接 ComboBox 信号 + 同步选项 |
| `OmniNav.cpp` | ~20 行 | 取消注释 Part 2 并改用新 API |

**总计约 90 行改动，不涉及新文件。**

---

## 注意事项

1. **性能**：只遍历有绑定的 Mesh，未绑定的 `continue` 跳过，开销极小
2. **解绑**：选择 "None" 时调用 `unbindTool()`，将 actor 的 UserMatrix 重置为绑定前的初始值
3. **Tool 删除**：如果用户删除了某个 Tool，需遍历 Mesh 表格，将所有引用该 Tool 的 ComboBox 重置为 "None" 并解绑
4. **单位**：Tool 的 finalMatrix 单位是 mm，Mesh 加载时的坐标也是 mm，无需单位转换
5. **2D 投影**：Mesh 的 `m_projectActors`（2D 投影）也需要同步更新，可作为第二阶段实现
