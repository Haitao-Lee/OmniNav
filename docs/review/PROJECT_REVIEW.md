# OmniNav 项目全面审查报告

**审查日期**: 2026-05-01  
**项目版本**: 0.1.0  
**作者**: Haitao-Lee

---

## 1. 项目概述

OmniNav 是一个基于 C++20 / Qt5 的医学影像导航应用，用于机器人辅助穿刺手术。系统集成了四个同步 VTK 渲染窗口（轴位、3D、冠状、矢状），NDI 光学追踪，以及 Universal Robots + 步进电机的穿刺工作流。

**技术栈**:
- 语言: C++20
- UI框架: Qt 5.15+
- 医学影像: VTK 9+, ITK 5+
- 数学库: Eigen3
- 构建系统: CMake 3.28+
- 包管理: vcpkg
- 目标平台: Windows 10/11 x64, Visual Studio 2022

---

## 2. 完整目录结构

```
OmniNav/
├── CMakeLists.txt                    # 根构建文件 (264行)
├── README.md                         # 项目文档 (698行)
├── CLAUDE.md                         # Claude Code 指导文件
├── .gitignore                        # Git 忽略规则
├── All_Landmarks.json                # 示例地标数据
├── All_Transforms.json               # 示例变换数据
├── nature_article.pdf                # 参考论文
├── 标定测试板.STL                     # 标定测试板模型
├── Model.zip                         # 模型压缩包
│
├── HeadNeckPhantom/                  # 测试体模数据
│   ├── CT/                           # DICOM 图像切片 (.IMA)
│   ├── DCM/                          # 额外 DICOM 数据
│   ├── exp0412/                      # 实验数据
│   ├── neck.stl                      # 颈部网格 (96 MB)
│   ├── tumor.stl                     # 肿瘤网格 (2.5 MB)
│   └── vessels.stl                   # 血管网格 (52 MB)
│
├── exp_slice_man0128/                # 切片实验数据
│   ├── silica_man.stl                # 硅胶体模 (49 MB)
│   └── All_Landmarks.json
│
└── SRC/                              # 所有源代码
    ├── main.cpp                      # 应用入口 (87行)
    ├── OmniNav.h / OmniNav.cpp       # 主应用类 (69/1225行)
    ├── config.json                   # 模块配置
    ├── demo.cpp                      # 已注释的 NDI 演示
    │
    ├── Base/                         # 主窗口和视图管理
    │   ├── Base.h / Base.cpp         # QMainWindow + VTK 视图 (135/1092行)
    │   ├── Display.h / Display.cpp   # 显示组件封装
    │   ├── Base.ui / Display.ui      # Qt Designer 文件
    │   ├── config.json               # 视图布局、颜色、相机配置
    │   └── resource/                 # 资源文件 (图标等)
    │
    ├── Item/                         # 数据模型类
    │   ├── Image.h / Image.cpp       # 医学图像 (ITK/VTK) (69/485行)
    │   ├── Model3D.h / Model3D.cpp   # 3D 网格模型 (57/232行)
    │   ├── Tool.h / Tool.cpp         # NDI 追踪工具 (105/253行)
    │   ├── Landmark.h / Landmark.cpp # 3D 地标点 (60/193行)
    │   └── OmniTransform.h/cpp       # 4x4 变换矩阵
    │
    ├── IO/                           # 硬件 I/O 线程
    │   ├── NDICombinedWorker.h/cpp   # NDI 追踪 QThread (61/250行)
    │   └── NDIWorker.h/cpp           # 旧版 NDI 工作线程 (已注释)
    │
    ├── Modules/
    │   ├── BaseModule/               # 模块基类
    │   │   ├── BaseModule.h / BaseModule.cpp
    │   │   └── config.json
    │   │
    │   ├── DataManager/              # 中央数据管理
    │   │   ├── DataManager.h / DataManager.cpp  (197/2628行)
    │   │   ├── DataManager.ui
    │   │   └── config.json           # 图像阈值、体渲染、颜色
    │   │
    │   ├── OpticalNavigation/        # NDI 光学追踪
    │   │   ├── OpticalNavigation.h / OpticalNavigation.cpp  (160/1136行)
    │   │   ├── OpticalNavigation.ui
    │   │   └── config.json           # NDI 设备 IP
    │   │
    │   ├── ElectromagneticNavigation/ # 预留模块 (空实现)
    │   │   ├── ElectromagneticNavigation.h / .cpp
    │   │   └── config.json
    │   │
    │   └── PunctureRobot/            # 机器人 + 步进电机控制
    │       ├── PunctureRobot.h / PunctureRobot.cpp  (179/858行)
    │       ├── RobotWorker.h / RobotWorker.cpp  (83/239行)
    │       ├── PunctureRobot.ui
    │       ├── config.json           # 机器人 IP、EEF 串口、法兰变换
    │       └── drivers/              # 步进电机驱动
    │           ├── usb_device.h / io.h / pwm.h / adc.h
    │           └── libs/windows/x86_64/  # 预编译库
    │
    ├── CombinedAPI/                  # NDI Combined API 封装 (34个文件)
    │   ├── CombinedApi.h/cpp         # 主 API 类
    │   ├── TcpConnection.h/cpp       # TCP 连接
    │   ├── ComConnection.h/cpp       # 串口连接
    │   ├── ToolData.h/cpp            # 工具追踪数据
    │   ├── SystemCRC.h/cpp           # CRC 校验
    │   └── Gbf*.h/cpp                # GBF 帧解析 (6个类)
    │
    ├── CombinedAPIsample/            # NDI 官方示例 SDK
    │   ├── library/                  # 头文件和源文件
    │   ├── sample/src/main.cpp       # 示例应用
    │   └── bin/win64/                # 预编译二进制
    │
    ├── ndicapi/                      # NDI C API (git 子模块)
    │   ├── ndicapi.cxx / ndicapi.h   # 核心 C API
    │   ├── ndicapi_serial_win32.cxx  # Win32 串口
    │   └── ndicapi_socket_win32.cxx  # Win32 套接字
    │
    ├── NewUrAPI/                     # Universal Robots RTDE API
    │   ├── URController.h/cpp        # UR 控制器类
    │   ├── RTDEInterface.h/cpp       # RTDE 通信
    │   ├── RobotTypes.h/cpp          # 类型定义
    │   ├── URMath.h/cpp              # 数学工具
    │   └── URScript.script           # UR 脚本模板
    │
    ├── Utils/                        # 共享工具
    │   ├── GeometryUtils.h/cpp       # 几何工具 (平面拟合、配准、ICP)
    │   └── VisualizationUtils.h/cpp  # VTK 可视化辅助
    │
    └── Cache/                        # 缓存目录 (空)
```

---

## 3. 构建系统分析

**文件**: `CMakeLists.txt` (264行)

### 关键特性
- CMake 3.28+，启用 `CMP0167 NEW` 策略
- C++20 标准，无扩展
- Qt Auto-MOC/RCC/UIC 自动处理
- 单一可执行文件，所有模块编译为一个二进制

### 依赖项

| 依赖 | 类型 | 用途 |
|------|------|------|
| Qt 5.15+ | vcpkg | Widgets, Network, Core, Gui, SerialPort |
| VTK 9+ | vcpkg | 渲染、OpenGL2、Qt 支持、成像、IO |
| ITK 5+ | vcpkg | 通用、IO (GDCM, NIFTI)、图像强度 |
| Eigen3 | vcpkg | 线性代数 |
| Boost | vcpkg | system, thread |
| spdlog | vcpkg | 日志 |
| NDI Combined API | 内置 | NDI 设备通信 |
| ndicapi | Git 子模块 | 底层 NDI C API |
| NewUrAPI | 内置 | UR RTDE 客户端 |
| librockmong | 预编译 | 步进电机 USB 驱动 |

### 构建命令

```powershell
# 创建构建目录并配置
mkdir build && cd build
cmake -S .. -B . -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake"

# 构建 Release
cmake --build . --config Release

# 构建输出: build/Release/OmniNav.exe
```

### 构建后命令
- 从 `drivers/libs/windows/x86_64/` 复制 DLL 到输出目录
- 复制 7 个配置 JSON 文件到输出目录
- 配置 `ndicapiExport.h` 模板

---

## 4. 模块架构

### 模块层次结构

```
QMainWindow
  └── BaseModule (基类，实现 init/splitter/button/config 模式)
        ├── DataManager        -- 中央数据枢纽
        ├── OpticalNavigation  -- NDI 追踪
        ├── ElectromagneticNavigation  -- 占位符 (空实现)
        └── PunctureRobot      -- 机器人 + 步进电机控制
```

### 模块加载机制

模块通过 `OmniNav_config.json` 中的 `modules` 数组动态加载。`OmniNav::addModule()` 使用硬编码的 `if/else if` 链将模块名称映射到具体类。

### 模块间通信

所有模块通过 Qt **信号/槽** 通信，在 `OmniNav::createActions()` 中连接。`OmniNav` 类充当中央中介者，将 DataManager 信号连接到 OpticalNavigation 和 PunctureRobot 的槽，反之亦然。

### 各模块职责

#### DataManager (数据管理器)
- **职责**: 中央数据枢纽，管理所有数据对象
- **数据对象**: Image, Model3D, Landmark, Tool, OmniTransform
- **功能**: 
  - 图像导入 (DICOM, NIfTI, MetaImage)
  - 3D 模型加载 (STL, OBJ, PLY)
  - 地标管理
  - 工具管理
  - 变换管理
  - 窗口/窗位调节
  - 体渲染控制

#### OpticalNavigation (光学导航)
- **职责**: NDI 光学追踪集成
- **功能**:
  - NDI 设备连接/断开
  - 工具 ROM 文件加载
  - 实时追踪数据获取
  - 采样 (收集追踪数据)
  - 配准 (点对点地标匹配)
  - 枢轴校准

#### PunctureRobot (穿刺机器人)
- **职责**: UR 机器人 + 步进电机控制
- **功能**:
  - UR 机器人连接/断开 (RTDE)
  - 步进电机连接/控制 (USB)
  - 轨迹规划
  - 穿刺工作流状态机
  - 手动 jog 控制

#### ElectromagneticNavigation (电磁导航)
- **职责**: 预留模块，目前为空实现
- **状态**: 仅包含空 UI 和空配置

---

## 5. 数据层 (Item/)

### 数据模型类

| 类 | 文件 | 用途 | 关键成员 |
|----|------|------|----------|
| **Image** | `Item/Image.h/cpp` | 医学图像封装 | vtkImageData, ITK reader, 3个重切演员 (轴/矢/冠), 1个体渲染演员, LUT/CTF/PWF |
| **Model3D** | `Item/Model3D.cpp` | 3D 网格 (STL/OBJ/PLY) | vtkPolyData, vtkActor, 2D 视图投影演员 |
| **Tool** | `Item/Tool.h/cpp` | NDI 追踪工具 | ROM 路径, 原始/配准/校准/最终矩阵, 方向, 半径/长度, vtkActor |
| **Landmark** | `Item/Landmark.cpp` | 3D 地标点 | 坐标, 点集 ID, vtkSphereSource 演员, 投影演员 |
| **OmniTransform** | `Item/OmniTransform.h/cpp` | 4x4 变换 | 名称 + Eigen::Matrix4d |

### Tool 矩阵流水线

```
rawMatrix * calibrationMatrix * registrationMatrix = finalMatrix
```

在 `updateFinalMatrix()` 中计算。

---

## 6. 硬件集成

### 6.1 NDI 光学追踪

- **工作线程**: `NDICombinedWorker` (QThread-based)
- **API**: `CombinedApi` 类封装 TCP 连接
- **底层**: `ndicapi` C 库处理串口/套接字通信
- **协议**: 二进制 BX/BX2 命令获取追踪数据，ASCII 用于配置
- **数据流**: 
  ```
  NDICombinedWorker::doTrackingWork()
    → capi.getTrackingDataBX()
    → 解析为 TrackedTool 结构体
    → 发射 allToolsUpdated() 信号
  ```
- **配置**: `OpticalNavigation_config.json` 中的 IP 地址 (默认: 192.168.1.202)

### 6.2 Universal Robots

- **工作线程**: `RobotWorker` (QThread-based)
- **API**: `URController` 类管理 RTDE 连接
- **协议**: RTDE (实时数据交换)，125 Hz
- **能力**: 
  - `MoveHardPose` - 硬位置移动
  - `MoveSoftPose` - 软位置移动
  - `MoveSoftPosOnly` - 仅位置软移动
  - `MoveSpeedPosOnly` - 速度位置移动
  - `Stop` - 停止
- **状态追踪**: `URStatus` 结构体跟踪关节位置、TCP 位姿/速度/力、机器人模式、安全模式、运行时状态
- **配置**: `PunctureRobot_config.json` 中的 IP (默认: 192.168.1.15)

### 6.3 步进电机

- **库**: `librockmong` (预编译 Windows x64 DLL/LIB)
- **C API**: 
  - `usb_device.h` - USB 设备扫描
  - `io.h` - I/O 控制
  - `pwm.h` - PWM 控制
  - `adc.h` - ADC 读取
- **集成**: `RobotWorker` 直接调用 `UsbDevice_Scan()` 和 PWM 函数
- **参数**: 
  - 电机螺距: 2mm
  - 每转步数: 200
  - 微步数: 8
- **控制**: `driver_StepperMove(velocity_mm_s)` 将 mm/s 转换为 PWM 频率

---

## 7. 配置文件

7 个 JSON 配置文件，构建时自动复制到可执行文件目录：

| 源路径 | 目标名称 | 内容 |
|--------|----------|------|
| `SRC/config.json` | `OmniNav_config.json` | 模块列表数组 |
| `SRC/Base/config.json` | `Base_config.json` | 视图模式、顺序、颜色、几何、相机 |
| `SRC/Modules/DataManager/config.json` | `DataManager_config.json` | HU 范围、阈值、体渲染属性、图像矩阵/法线、地标/网格/工具默认值 |
| `SRC/Modules/OpticalNavigation/config.json` | `OpticalNavigation_config.json` | NDI 设备 IP |
| `SRC/Modules/PunctureRobot/config.json` | `PunctureRobot_config.json` | 机器人 IP、EEF 串口、法兰到 EEF 变换、运动参数 |
| `SRC/Modules/ElectromagneticNavigation/config.json` | `ElectromagneticNavigation_config.json` | 空 |
| `SRC/Modules/BaseModule/config.json` | `BaseModule_config.json` | 空 |

---

## 8. UI 文件

7 个 `.ui` 文件由 CMake AUTOUIC 处理：

| 文件 | 类 | 用途 |
|------|-----|------|
| `Base/Base.ui` | `Ui::BaseClass` | 主窗口：菜单栏、工具栏、4 路分割器视图、模块面板、状态栏 |
| `Base/Display.ui` | `Ui::DisplayClass` | 每个视图的组件：VTK 渲染组件、滚动条、标签 |
| `Modules/BaseModule/BaseModule.ui` | `Ui::BaseModuleClass` | 默认/欢迎模块面板 |
| `Modules/DataManager/DataManager.ui` | `Ui::DataManagerClass` | 图像/网格/地标/工具/变换表格、窗口/窗位滑块、体渲染控件 |
| `Modules/OpticalNavigation/OpticalNavigation.ui` | `Ui::OpticalNavigationClass` | 连接/追踪按钮、工具 ROM 加载、采样、配准、校准面板 |
| `Modules/PunctureRobot/PunctureRobot.ui` | `Ui::PunctureRobotClass` | 机器人连接、步进电机连接、轨迹规划、jog 控制、穿刺工作流 |
| `Modules/ElectromagneticNavigation/ElectromagneticNavigation.ui` | `Ui::ElectromagneticNavigationClass` | 占位符 |

---

## 9. 数据流架构

### 图像导入流程

```
用户文件导入
      │
      ▼
DataManager::addImage() ──创建──> Image 对象 (ITK 读取 + VTK 管道)
      │
      ├─ 发射 signalAddImage ──> OmniNav::onAddImage()
      │                              ├─ 添加演员到 2D 渲染器 (轴/矢/冠)
      │                              ├─ 添加体渲染到 3D 渲染器
      │                              ├─ 重置相机
      │                              └─ 更新十字线
```

### 追踪数据流程

```
NDI 设备 (TCP)
      │
      ▼
NDICombinedWorker::doTrackingWork() ──轮询──> CombinedApi::getTrackingDataBX()
      │
      ├─ 发射 allToolsUpdated(QVector<TrackedTool>)
      │
      ▼
OmniNav::onUpdateNavigationInfo2DataManager()
      ├─ 更新工具表格 UI (句柄、RMSE、位置、在线状态)
      ├─ 更新 3D 视图中的工具演员
      │
PunctureRobot::onToolsUpdated()
      ├─ 读取追踪工具位姿
      ├─ 从地标 + 配准计算机器人目标
      └─ 通过 RobotWorker 发送命令 ──> URController (RTDE)
                                     ──> StepperController (USB PWM)
```

---

## 10. 测试数据

项目包含真实的临床/研究测试数据：

- **HeadNeckPhantom/**: DICOM CT 图像、颈部/肿瘤/血管 STL 网格、地标
- **exp_slice_man0128/**: 硅胶体模 STL 模型 + 地标
- **All_Landmarks.json** / **All_Transforms.json**: 项目根目录的示例数据
- **标定测试板.STL**: 标定测试板 (中文文件名)

---

## 11. 代码质量评估

### 优点

1. **良好的模块化架构**: 清晰的关注点分离，每个模块独立且职责明确
2. **完善的 README**: 包含构建说明、架构图、故障排除 (698行)
3. **正确的信号/槽使用**: 模块间通过 Qt 信号/槽解耦通信
4. **VTK 管道良好**: 重切、体渲染、十字线同步
5. **JSON 配置系统**: 构建时自动复制，便于配置管理
6. **QThread 硬件 I/O**: 保持 UI 响应性
7. **Eigen 数学库**: 整个项目中统一使用 Eigen 进行矩阵运算

### 问题和关注点

#### 严重问题

1. **重复的嵌套副本**: `SRC/OmniNav/SRC/` 包含整个源树的完整副本，显著增加仓库大小

2. **原始指针所有权**: `DataManager` 以原始指针存储数据对象 (`std::vector<Image*>`, `std::vector<Tool*>` 等)，没有明确的所有权语义。析构函数似乎不删除它们，存在内存泄漏风险

3. **`QCoreApplication::processEvents()` 使用**: 在 `onAddImage()` 中大量调用，可能导致重入问题

4. **缺少错误处理**: 许多 VTK/ITK 操作缺少空指针检查或错误处理

#### 中等问题

5. **硬编码模块加载**: `OmniNav::addModule()` 使用硬编码的 `if/else if` 链，而不是工厂模式或注册系统。添加新模块需要修改此函数

6. **GLOB 源文件收集**: CMakeLists.txt 使用 `file(GLOB ...)`，不重新运行 CMake 就无法检测新文件

7. **仅限 Windows 路径**: vcpkg 工具链路径是相对的 (`../../vcpkg/`)，ndicapi 源仅包含 `_win32` 变体，直接链接 `ws2_32`，librockmong 仅 Windows x64

8. **大型单体文件**: 
   - `DataManager.cpp`: 2628 行
   - `OmniNav.cpp`: 1225 行
   - 这些文件应该被分解

9. **缺少单元测试**: README 提到测试，但没有测试框架或测试文件

#### 轻微问题

10. **配置键拼写错误**: `"cornal"` 应为 `"coronal"`，贯穿整个代码库

11. **混合命名约定**: 
    - `snake_case`: `loadConfig`
    - `camelCase`: `onModuleSelectionChanged`
    - `PascalCase`: `AddViewProp`
    - Qt 自动连接: `on_connectRobot_btn_clicked`

12. **大量注释代码**: 
    - `OmniNav.cpp` 有约 100 行注释代码
    - `NDIWorker.h` 和 `demo.cpp` 完全注释

13. **空函数体**: 
    - `onUpdate3DOpacity()` 在 `OmniNav.cpp:1072-1074` 为空
    - `onDeleteImage()` 未实现

14. **ElectromagneticNavigation 是存根**: 模块被加载但没有功能

15. **编译问题补丁**: `PunctureRobot.h` 中的 `[Fix C2065]` 和 `[Fix C2039]` 注释表明编译问题被修补而非正确解决

---

## 12. 改进建议

### 高优先级

1. **移除重复的 `SRC/OmniNav/` 副本** - 不必要地使仓库大小翻倍
2. **实现正确的内存管理** - 对 DataManager 中的 Item 对象使用 `std::unique_ptr` 或 `std::shared_ptr`
3. **替换 `processEvents()` 调用** - 使用正确的异步模式或排队连接
4. **添加错误处理** - 在 VTK/ITK 操作中添加空指针检查和异常处理

### 中优先级

5. **在 CMakeLists.txt 中用显式文件列表替换 GLOB** - 确保可靠构建
6. **实现模块工厂/注册模式** - 替换硬编码的 if/else 链
7. **清理死代码** - 移除注释的 NDIWorker、demo.cpp 和 OmniNav.cpp 中的大型注释块
8. **修复 "cornal" 拼写错误** - 更正为 "coronal"
9. **分解大型文件** - DataManager.cpp 和 OmniNav.cpp 应该被拆分

### 低优先级

10. **添加跨平台支持** - 抽象平台特定代码，使用 CMake 生成器表达式复制 DLL
11. **添加单元测试** - 使用 Google Test 或 Catch2
12. **添加 LICENSE 文件** - 如 README 中推荐
13. **实现空函数** - `onDeleteImage()` 和 `onUpdate3DOpacity()`
14. **统一命名约定** - 选择一种风格并保持一致

---

## 13. 总结

OmniNav 是一个功能丰富的医学影像导航应用，具有良好的模块化架构和完整的硬件集成。项目的核心设计（信号/槽通信、QThread 硬件 I/O、JSON 配置）是合理的。

主要的技术债务在于：
- 内存管理（原始指针）
- 代码组织（大型单体文件）
- 构建系统（GLOB 收集）
- 平台依赖（仅 Windows）

建议按优先级逐步改进，首先解决内存泄漏和重复代码问题，然后重构大型文件和改进构建系统。

---

*报告生成时间: 2026-05-01*
