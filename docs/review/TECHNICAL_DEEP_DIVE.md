# OmniNav 核心机制深度分析

## 1. Navigation 与 Robot 的通信机制

### 1.1 通信架构总览

Navigation（OpticalNavigation）和 Robot（PunctureRobot）之间**不直接通信**，而是通过 **OmniNav 中央中介者** 以 Qt 信号/槽机制进行间接通信。

```
                    ┌─────────────────────────────────────┐
                    │           OmniNav (中介者)            │
                    │     createActions() 中连接所有信号     │
                    └──────┬──────────────────┬────────────┘
                           │                  │
            signalAllToolsUpdated         reqGetTools
            (追踪数据广播)              (数据请求/响应)
                           │                  │
                           ▼                  ▼
                ┌──────────────────┐  ┌──────────────────┐
                │ OpticalNavigation │  │  PunctureRobot   │
                │   (光学导航)      │  │   (穿刺机器人)    │
                └──────────────────┘  └──────────────────┘
```

### 1.2 核心通信链路

#### 链路 1：追踪数据广播（Navigation → Robot）

**连接代码** (`OmniNav.cpp:348`):
```cpp
connect(opticalNavigation.get(), &OpticalNavigation::signalAllToolsUpdated, 
        punctureRobot.get(), &PunctureRobot::onToolsUpdated);
```

**数据流**:
1. `NDICombinedWorker::doTrackingWork()` 每 16ms (60Hz) 轮询 NDI 设备
2. 解析 BX 命令返回的二进制数据为 `TrackedTool` 结构体
3. 发射 `allToolsUpdated(QVector<TrackedTool>)` 信号
4. `OpticalNavigation::onToolDataReceived()` 接收：
   - 更新 Tool 的 rawMatrix
   - 发射 `signalAllToolsUpdated(tools)` 信号
5. **两个接收者同时收到**：
   - `OmniNav::onUpdateNavigationInfo2DataManager()` → 更新 UI 表格
   - `PunctureRobot::onToolsUpdated()` → 更新机器人控制

#### 链路 2：数据请求/响应（Robot → Navigation 通过 DataManager）

**连接代码** (`OmniNav.cpp:384-386`):
```cpp
connect(punctureRobot.get(), &PunctureRobot::reqGetTools, 
        [=](std::vector<Tool*>& tools){
            tools = dataManager->getTools(); 
        });
```

**机制**: Robot 发射 `reqGetTools` 信号，OmniNav 的 lambda 槽函数从 DataManager 获取工具列表并填充到引用参数中。这是一种**同步请求/响应模式**。

#### 链路 3：地标数据共享

Robot 需要获取轨迹地标（Entry/Target 点）：
```cpp
// OmniNav.cpp:367-376
connect(punctureRobot.get(), &PunctureRobot::reqGetLandmarks, 
        [=](std::vector<Landmark*>& landmarks, QString pointSetName){
            landmarks.clear();
            const std::vector<Landmark*>& allLandmarks = dataManager->getLandmarks();
            for (auto lm : allLandmarks) {
                if (lm && QString::fromStdString(lm->getPointset()) == pointSetName) {
                    landmarks.push_back(lm);
                }
            }
        });
```

### 1.3 机器人状态机与追踪数据的耦合

`PunctureRobot::onToolsUpdated()` 是核心回调，每次追踪数据更新时执行：

```
onToolsUpdated(tools)
    │
    ├─ 1. 更新 m_toolCache（本地工具缓存）
    │
    ├─ 2. 更新 UI 下拉框颜色（绿=可见，红=丢失）
    │
    ├─ 3. 穿刺闭环控制（NeedleState::INSERTING/RETRACTING）
    │     ├─ 获取针尖当前位姿
    │     ├─ 计算到目标的 1D 投影距离
    │     ├─ 分段减速控制（>15mm 全速，0.5-15mm 减速，<0.5mm 停止）
    │     └─ 发送速度命令给步进电机
    │
    └─ 4. 标定数据收集（m_isCollectingCalib）
          ├─ 收集 50 帧 probe + needle 位姿
          └─ 调用 performHandEyeCalibration()
```

---

## 2. Tool 的工作机制

### 2.1 Tool 的生命周期

#### 创建阶段
用户在 DataManager UI 中点击"Add Tool"，选择 `.rom` 文件：
```
DataManager::addTool(romPath)
    → new Tool(romPath)
    → Tool 构造函数：
        ├─ 从 ROM 路径提取名称
        ├─ 生成针状几何体 (getNeedlePolyData)
        ├─ 初始化 4 个变换矩阵为单位矩阵
        └─ 创建 VTK Actor
    → 发射 signalAddTool(tool)
    → OmniNav::onAddTool() 将 Actor 添加到 3D 渲染器
```

#### 追踪阶段
NDI 设备返回追踪数据后：
```
NDICombinedWorker::doTrackingWork()
    → 解析 BX 数据为 TrackedTool（含 Eigen::Matrix4d）
    → 发射 allToolsUpdated

OpticalNavigation::onToolDataReceived()
    → updateToolMatrices()
        → 匹配 TrackedTool.name == Tool.name
        → Tool::setRawMatrix(vtkMat)
            → updateFinalMatrix()
```

### 2.2 Tool 的矩阵流水线

Tool 内部维护 **4 个变换矩阵**，最终合成 `finalMatrix`：

```
┌─────────────┐    ┌──────────────────┐    ┌──────────────────┐
│  rawMatrix   │ ×  │ calibrationMatrix │ ×  │   regMatrix      │
│  (NDI 原始)  │    │  (校准矩阵)       │    │  (配准矩阵)       │
└──────┬──────┘    └────────┬─────────┘    └────────┬─────────┘
       │                    │                        │
       └────────────────────┼────────────────────────┘
                            │
                            ▼
                   ┌──────────────────┐
                   │   finalMatrix     │
                   │  (最终位姿)       │
                   └──────────────────┘
```

**代码实现** (`Tool.cpp:201-248`):
```cpp
void Tool::updateFinalMatrix() {
    // 1. Final = Reg × Raw × Cali
    vtkSmartPointer<vtkMatrix4x4> tempMat = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4(m_rawMatrix, m_calibrationMatrix, tempMat);
    vtkMatrix4x4::Multiply4x4(m_regMatrix, tempMat, m_finalMatrix);
    
    // 2. ActorMat = Final × Correction（方向修正）
    // 根据 m_direction（Z+/Z-/X+/X-/Y+/Y-）旋转几何体
    vtkSmartPointer<vtkTransform> correctionTransform = vtkSmartPointer<vtkTransform>::New();
    if (m_direction == "Z+") correctionTransform->RotateX(180);
    // ... 其他方向
    
    vtkMatrix4x4::Multiply4x4(m_finalMatrix, correctionTransform->GetMatrix(), m_actorMatrix);
}
```

### 2.3 各矩阵的来源

| 矩阵 | 来源 | 设置时机 |
|------|------|----------|
| `rawMatrix` | NDI 追踪数据 | 每帧更新（60Hz） |
| `calibrationMatrix` | 枢轴校准/Tip-to-Tip 校准 | 用户执行校准后 |
| `regMatrix` | 地标配准（ICP/SVD） | 用户执行配准后 |
| `finalMatrix` | 自动计算 | 任一矩阵变化时 |
| `actorMatrix` | 自动计算（含方向修正） | finalMatrix 变化时 |

### 2.4 Tool 的可视化

Tool 不是加载 STL 文件，而是**程序化生成针状几何体**：

```cpp
// Tool.cpp:24
m_polydata = VisualizationUtils::getNeedlePolyData(m_radius, m_length, 
                                                    QString::fromStdString(m_direction));
```

生成的针状体参数：
- `m_radius`: 默认 5.0（单位待确认）
- `m_length`: 默认 250.0
- `m_direction`: 默认 "Z+"，可选 Z-/X+/X-/Y+/Y-

针状体的几何方向与追踪坐标系对齐，通过 `correctionTransform` 旋转实现。

---

## 3. Tool 与 STL 模型的关联机制

### 3.1 当前状态：关联功能已注释

**关键发现**：`OmniNav.cpp` 中 `onUpdateNavigationInfo2DataManager()` 函数的 Part 2 和 Part 3（约 100 行代码）已被注释掉。这两部分正是实现 Tool 与 STL/Landmark 关联的核心逻辑。

#### Part 2：Mesh 跟随 Tool（已注释）

```cpp
// OmniNav.cpp:889-915 (已注释)
for (int i = 0; i < meshTable->rowCount(); ++i) {
    Model3D* mesh = meshes[i];
    QComboBox* cbb = meshTable->cellWidget(i, 5)->findChild<QComboBox*>(); // Column 5 = Tool ComboBox
    QString selectedToolName = cbb->currentText();
    
    if (selectedToolName != "None") {
        Tool* tool = toolCache[selectedToolName];
        vtkMatrix4x4* toolMat = tool->getFinalMatrix();
        mesh->getActor()->SetUserMatrix(toolMat);  // STL 跟随 Tool 移动
    }
}
```

**设计意图**：DataManager 的 Mesh 表格中，每个 STL 模型有一个 Tool 下拉框。当选择某个 Tool 后，该 STL 模型的 Actor 会被设置 `SetUserMatrix(toolMat)`，从而在 3D 视图中跟随 Tool 实时移动。

#### Part 3：Landmark 跟随 Tool（已注释）

```cpp
// OmniNav.cpp:920-986 (已注释)
for (int i = 0; i < landmarkTable->rowCount(); ++i) {
    Landmark* lm = landmarks[i];
    QComboBox* cbb = landmarkTable->cellWidget(i, 6)->findChild<QComboBox*>(); // Column 6 = Tool ComboBox
    QString selectedToolName = cbb->currentText();
    
    if (selectedToolName != "None") {
        Tool* tool = toolCache[selectedToolName];
        vtkMatrix4x4* toolMat = tool->getFinalMatrix();
        
        // 直接修改地标坐标为 Tool 的位置
        double newCoords[3] = {
            toolMat->GetElement(0, 3),
            toolMat->GetElement(1, 3),
            toolMat->GetElement(2, 3)
        };
        lm->setCoordinates(newCoords);
    }
}
```

**设计意图**：Landmark 表格中，每个地标也有一个 Tool 下拉框。选择 Tool 后，地标的坐标会被实时更新为 Tool 的位置。

### 3.2 关联的数据结构

#### DataManager 中的表格列定义

**Mesh 表格** (`DataManager.ui`):
| 列号 | 内容 | 说明 |
|------|------|------|
| 0 | Name | 模型名称 |
| 1 | File Path | 文件路径 |
| 2 | Color | 颜色按钮 |
| 3 | Opacity | 透明度 |
| 4 | Visibility | 可见性 |
| **5** | **Tool ComboBox** | **关联的 Tool（用于跟随）** |

**Landmark 表格**:
| 列号 | 内容 | 说明 |
|------|------|------|
| 0 | Name | 地标名称 |
| 1 | Coordinates | 坐标 |
| 2 | Color | 颜色按钮 |
| 3-5 | ... | 其他属性 |
| **6** | **Tool ComboBox** | **关联的 Tool（用于跟随）** |

### 3.3 关联的工作流程（设计意图）

```
1. 用户导入 CT 图像 → DataManager::addImage()
2. 用户导入 STL 模型 → DataManager::add3Dmodel()
3. 用户添加 Tool（.rom 文件）→ DataManager::addTool()
4. 用户在 Mesh 表格中，为某个 STL 选择关联的 Tool
5. 开始追踪后：
   ├─ NDI 返回 Tool 的实时位姿
   ├─ Tool::setRawMatrix() → updateFinalMatrix()
   └─ OmniNav::onUpdateNavigationInfo2DataManager() (Part 2)
       → mesh->getActor()->SetUserMatrix(toolMat)
       → STL 模型在 3D 视图中跟随 Tool 移动
```

### 3.4 Landmark 与 Tool 的特殊关联：采样

Landmark 与 Tool 的关联不仅用于"跟随"，更重要的是用于**配准采样**：

```
采样流程 (OpticalNavigation::processSamplingData):
1. 用户选择 Probe Tool 和 Input/Output Point Set
2. 点击 "Start Sampling"
3. 每次追踪数据更新时：
   ├─ 检查 Probe 是否稳定（1.0mm 容差，1500ms 持续时间）
   ├─ 稳定后，记录 Probe 的位置作为新 Landmark
   ├─ 自动计算 Source（采样点）和 Target（输入点集）之间的配准矩阵
   └─ 将配准矩阵设置为 Probe 的 regMatrix
4. 配准完成后，Tool 的 finalMatrix 会自动更新
```

---

## 4. 坐标系变换全链路

### 4.1 坐标系定义

```
┌──────────────────────────────────────────────────────────────┐
│                    NDI World（追踪器世界坐标系）                │
│                    单位：mm                                    │
└──────────────────────────┬───────────────────────────────────┘
                           │
                    Registration Matrix
                    （配准矩阵：患者坐标系 → NDI 坐标系）
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│                  Patient/CT（患者/CT 坐标系）                  │
│                  单位：mm（与 DICOM 一致）                     │
└──────────────────────────┬───────────────────────────────────┘
                           │
                    Image Spacing/Origin
                    （图像体素 → 物理坐标）
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│                    Image Voxel（图像体素坐标系）                │
│                    单位：体素                                  │
└──────────────────────────────────────────────────────────────┘
```

### 4.2 Robot 坐标系

```
┌──────────────────────────────────────────────────────────────┐
│                  Robot Base（机器人基座坐标系）                 │
│                  单位：m（UR 机器人标准）                       │
└──────────────────────────┬───────────────────────────────────┘
                           │
                    World-to-Base Matrix
                    （m_matWorldToBase，通过手眼标定获得）
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│                  NDI World（追踪器世界坐标系）                  │
│                  单位：mm → 需要转换为 m                       │
└──────────────────────────────────────────────────────────────┘
```

### 4.3 关键变换矩阵

| 矩阵 | 含义 | 维度 | 单位 |
|------|------|------|------|
| `rawMatrix` | NDI 返回的 Tool 位姿 | 4×4 | mm |
| `calibrationMatrix` | 枢轴校准偏移 | 4×4 | mm |
| `regMatrix` | 患者配准（NDI→CT） | 4×4 | mm |
| `finalMatrix` | 最终 Tool 位姿 | 4×4 | mm |
| `m_matFlangeToTCP` | 法兰到末端执行器 | 4×4 | m |
| `m_matWorldToBase` | NDI 世界到机器人基座 | 4×4 | mm→m |

### 4.4 Robot 运动规划中的坐标转换

`PunctureRobot::on_URAction_btn_clicked()` 中的关键转换：

```cpp
// 1. 获取 NDI 坐标系下的目标点（mm）
Eigen::Vector3d pEntryW_mm, pTargetW_mm;
getLandmarksAndTrajectory(pEntryW_mm, pTargetW_mm);

// 2. 转换为米（UR 机器人使用米）
Eigen::Vector3d pEntryW_m = pEntryW_mm * 0.001;
Eigen::Vector3d pTargetW_m = pTargetW_mm * 0.001;

// 3. 计算机器人基座坐标系下的目标位置
// Tip_Target = Tip_Current + R_BW * (Target_W - Current_W)
Eigen::Vector3d pTipApproach_Base = pTipCurr_Base + R_BW * (pApproachW_m - pCurrentTipW_m);

// 4. 补偿法兰到针尖的杠杆臂
// Flange_Target = Tip_Target - (R_Target * vOffset_Flange)
m_targetApproach = pTipApproach_Base - vOffset_Rotated_Target;
```

---

## 5. 穿刺工作流状态机

```
                    ┌─────────────┐
                    │  PHASE_IDLE │ ◄──────────────────────────────┐
                    └──────┬──────┘                                │
                           │ on_URAction_btn_clicked()             │
                           ▼                                       │
                ┌──────────────────────┐                           │
                │ PHASE_MOVING_TO_     │                           │
                │     APPROACH         │                           │
                └──────────┬───────────┘                           │
                           │ dist < tolerance                      │
                           ▼                                       │
                ┌──────────────────────┐                           │
                │ PHASE_MOVING_TO_     │                           │
                │      ENTRY           │                           │
                └──────────┬───────────┘                           │
                           │ dist < tolerance                      │
                           ▼                                       │
                ┌──────────────────────┐     on_EFAction_btn      │
                │    PHASE_READY       │ ──────────────────────┐  │
                └──────────┬───────────┘                       │  │
                           │                                   ▼  │
                           │                          ┌───────────────┐
                           │                          │ NeedleState:: │
                           │                          │   INSERTING   │
                           │                          └───────┬───────┘
                           │                                  │
                           │                          distToGo1D ≤ 0
                           │                                  │
                           │                                  ▼
                           │                          ┌───────────────┐
                           │     on_ReturnAction      │ NeedleState:: │
                           │ ──────────────────────►  │   RETRACTING  │
                           │                          └───────┬───────┘
                           │                                  │
                           │                          travel ≤ 1.5mm
                           │                                  │
                           ▼                                  ▼
                ┌──────────────────────┐            ┌──────────────────┐
                │ PHASE_RETURNING_     │ ◄──────────│ PHASE_RETURNING_ │
                │      ROBOT           │            │     NEEDLE       │
                └──────────┬───────────┘            └──────────────────┘
                           │ dist < tolerance
                           └──────────────────────────────►
```

---

## 6. 总结

### 6.1 通信模式

| 模式 | 实现 | 用途 |
|------|------|------|
| **广播** | `signalAllToolsUpdated` → 多个槽 | 追踪数据分发给 UI + Robot |
| **请求/响应** | `reqGetTools` + lambda 槽 | Robot 按需获取工具列表 |
| **状态同步** | `m_toolCache` 本地缓存 | Robot 维护最新工具状态 |

### 6.2 Tool 关联机制

| 关联类型 | 状态 | 机制 |
|----------|------|------|
| Mesh ↔ Tool | **已注释（未启用）** | `SetUserMatrix(toolMat)` |
| Landmark ↔ Tool | **已注释（未启用）** | `setCoordinates(toolPos)` |
| Tool ↔ 采样 | **已启用** | 稳定性检测 + 自动配准 |
| Tool ↔ 校准 | **已启用** | 枢轴/Tip-to-Tip 校准 |

### 6.3 关键发现

1. **Mesh/Landmark 跟随功能被注释**：设计意图是让 STL 模型和地标跟随 Tool 实时移动，但代码已被注释。可能原因：
   - 性能问题（每帧更新所有 Mesh 的 UserMatrix）
   - 逻辑问题（注释中提到 `setCoordinates` 会改变原始数据）
   - 开发中功能（尚未完成调试）

2. **Tool 不是 STL**：Tool 是程序化生成的针状体，不是加载的 STL 文件。STL 文件（如 tumor.stl、vessels.stl）是独立的 Model3D 对象。

3. **坐标系单位不一致**：NDI 使用 mm，UR 机器人使用 m，代码中有多处 `* 0.001` 转换。

---

*分析基于代码版本: 2026-05-01*
