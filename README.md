# OmniNav - 医疗影像导航与机器人穿刺系统

[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![Qt5](https://img.shields.io/badge/Qt-5-green.svg)](https://www.qt.io/)
[![VTK](https://img.shields.io/badge/VTK-9+-orange.svg)](https://vtk.org/)
[![ITK](https://img.shields.io/badge/ITK-5+-red.svg)](https://itk.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

> **OmniNav** 是一个模块化、跨平台的医疗影像导航应用程序，集成了多视图 VTK 可视化、光学跟踪（NDI Combined API）和机器人穿刺工作流（Universal Robots + 步进电机末端执行器）。支持 DICOM/NIfTI 医学影像导入、三维模型渲染、工具注册校准、实时跟踪和机器人控制。

---

## 📑 目录
- [核心特性](#核心特性)
- [系统架构](#系统架构)
- [技术栈](#技术栈)
- [硬件要求](#硬件要求)
- [依赖安装](#依赖安装)
- [构建指南](#构建指南)
- [配置文件详解](#配置文件详解)
- [快速入门](#快速入门)
- [模块详解](#模块详解)
- [数据格式支持](#数据格式支持)
- [开发指南](#开发指南)
- [故障排除](#故障排除)
- [测试与验证](#测试与验证)
- [贡献指南](#贡献指南)
- [许可证](#许可证)

---

## 🎯 核心特性

### 影像可视化
- ✅ **四视图同步**：轴位（Axial）、3D、冠状位（Coronal）、矢状位（Sagittal）
- ✅ **十字线联动**：跨视图同步导航，点击任意视图自动同步其他视图
- ✅ **滚动浏览**：支持鼠标滚轮或滑块逐层浏览
- ✅ **窗宽窗位调节**：实时调整影像对比度

### 数据导入/导出
- 📁 **医学影像**：DICOM（文件夹或单文件 `.dcm`）、NIfTI（`.nii`/`.nii.gz`）、NRRD、MHD/MHA
- 🧊 **三维模型**：STL、OBJ、PLY、VTK、VTP、G（用于网格/表面）
- 📍 **工具定义**：NDI `.rom` 文件（工具几何与 markers）
- 📊 **标定数据**：JSON 格式的 landmarks、transforms 可导入/导出
- 📝 **文本**：`.txt` 导入到信息面板

### 光学导航（NDI）
- 🔗 **设备连接**：TCP/IP 连接 NDI 光学跟踪系统（Polaris、Spectra、Vega 等）
- 📍 **工具追踪**：实时获取 6D 位姿（位置+姿态）
- 🎯 **采样模式**：采集多帧数据计算平均位姿
- 🔄 **注册（Registration）**：将工具坐标系与影像坐标系对齐
- ⚙️ **校准（Calibration）**：工具端到端（tip）变换校准

### 机器人穿刺工作流
- 🤖 **UR 机器人控制**：通过 RTDE 接口连接 Universal Robots 机械臂
- 🛠️ **步进电机驱动**：控制穿刺针的进给（通过 `librockmong` 驱动 USB 设备）
- 🔧 **末端执行器配置**： flange-to-EEF 变换矩阵，支持自定义工具
- 📋 **工作流向导**：引导式穿刺规划与执行流程

### 用户界面
- 🧩 **模块化设计**：每个功能模块独立，可通过配置文件启用/禁用
- 📊 **表格管理**：landmarks、tools、transforms 以表格形式管理
- 🎨 **可定制外观**：视图颜色、透明度、相机参数均可配置
- 💾 **配置持久化**：JSON 配置文件自动复制到可执行文件旁

---

## 🏗️ 系统架构

```mermaid
flowchart LR
    UI[OmniNav UI] --> DM[DataManager]
    UI --> ON[OpticalNavigation]
    UI --> EM[ElectromagneticNavigation]
    UI --> PR[PunctureRobot]

    DM --> Items["Image / Model3D / Landmark / Tool / OmniTransform"]
    DM --> VTK["VTK Rendering + ITK/VTK IO"]

    ON --> NDIW["NDIWorker (QThread)"]
    NDIW --> CAPI["CombinedApi / ndicapi"]
    NDIW --> SOCK["TCP Socket"]

    PR --> UR["URWorker (QThread)"]
    UR --> RTDE["UR RTDE"]
    UR --> STEP["Stepper Driver"]
    STEP --> USB[librockmong (USB)]
```

### 架构说明

- **主窗口（Base）**：Qt 主框架，管理菜单、工具栏、模块加载、视图布局
- **数据层（DataManager）**：统一管理影像、模型、landmarks、tools、transforms，提供数据共享
- **渲染层（VTK）**：基于 OpenGL 的高性能 3D 渲染，支持体绘制与面绘制
- **模块系统**：每个功能模块（OpticalNavigation、PunctureRobot 等）独立编译，通过接口与主窗口通信
- **线程模型**：硬件 I/O（NDI、UR、步进电机）在独立 QThread 中运行，避免阻塞 UI
- **配置系统**：JSON 配置文件定义模块参数，构建时复制到输出目录

---

## 🔧 技术栈

| 组件 | 版本 | 用途 |
|------|------|------|
| **C++** | C++20 | 核心语言，使用概念、协程（可选）、模块（未来） |
| **Qt** | 5.15+ | GUI 框架（Widgets）、网络、串口、线程 |
| **VTK** | 9.0+ | 3D 渲染、图像处理、数据可视化 |
| **ITK** | 5.0+ | 医学影像 IO（DICOM、NIfTI）与预处理 |
| **NDI API** | Combined API | 光学跟踪设备通信 |
| **UR RTDE** | 1.0+ | Universal Robots 实时数据交换 |
| **spdlog** | 1.x | 高性能日志系统 |
| **Eigen3** | 3.4+ | 线性代数（矩阵运算） |
| **Boost** | 1.70+ | 系统、线程库 |
| **librockmong** | - | 步进电机 USB 驱动（Windows x64） |
| **CMake** | 3.28+ | 构建系统 |
| **vcpkg** | - | 依赖包管理（Windows） |

---

## 🖥️ 硬件要求

### 必需硬件（临床/研究场景）
- **光学跟踪系统**：NDI Polaris / Spectra / Vega（提供 6D 位姿）
- **穿刺机器人**：Universal Robots（UR3/UR5/UR10 等）
- **步进电机驱动**：自定义 EEF 用于针的进给（通过 USB 控制）
- **工具 Marker**：NDI 反射球Marker套装（至少1个）
- **穿刺针**：安装Marker的专用穿刺针

### 推荐配置
- **操作系统**：Windows 10/11 64-bit（已测试）
- **GPU**：NVIDIA GeForce GTX 1060 或更高（VTK OpenGL 渲染）
- **内存**：16 GB RAM（处理大型医学影像）
- **存储**：SSD，至少 50 GB 可用空间
- **网络**：千兆以太网（NDI、UR 机器人、PC 在同一子网）

---

## 📦 依赖安装

### Windows（Visual Studio 2022 + vcpkg）

1. **安装 Visual Studio 2022**（Desktop development with C++ 工作负载）
2. **安装 vcpkg**：
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```
3. **安装依赖**：
   ```powershell
   .\vcpkg install qt5-base[core,network,gui,serialport,widgets]:x64-windows
   .\vcpkg install vtk[core,rendering,renderingopengl2,guisupportqt,imaging,ioimage]:x64-windows
   .\vcpkg install itk[common,ioimagebase,iogdcm,ionifti,imageintensity]:x64-windows
   .\vcpkg install eigen3:x64-windows
   .\vcpkg install boost-system boost-thread:x64-windows
   .\vcpkg install spdlog:x64-windows
   ```
4. **NDI Combined API**：
   - 从 NDI 官网下载 Combined API SDK
   - 解压到 `OmniNav/SRC/CombinedAPIsample/`（或自定义路径，修改 CMake）
5. **UR RTDE 库**：
   - 已包含 `OmniNav/SRC/NewUrAPI/`，无需额外安装
6. **librockmong**：
   - 已包含预编译库：`OmniNav/SRC/Modules/PunctureRobot/drivers/libs/windows/x86_64/librockmong.lib`
   - 对应的 DLL 在 `drivers/libs/windows/x86_64/`，构建时自动拷贝

### Linux（Ubuntu 22.04+）

```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install qtbase5-dev qtdeclarative5-dev libqt5serialport5-dev
sudo apt install libvtk9-dev libitk-dev
sudo apt install libeigen3-dev libboost-system-dev libboost-thread-dev
sudo apt install libspdlog-dev
```

NDI 和 UR 库需从官方获取。

---

## 🔨 构建指南

### Windows（Visual Studio 2022）

```powershell
# 克隆仓库
git clone https://github.com/Haitao-Lee/OmniNav.git
cd OmniNav

# 创建构建目录
mkdir build && cd build

# 配置 CMake（修改 vcpkg 路径）
cmake -S .. -B . `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"

# 构建（Release 模式）
cmake --build . --config Release

# 可执行文件位置
# build/Release/OmniNav.exe
```

### 命令行快速构建（MSVC）

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### 生成 IDE 项目（CLion / VS Code）

```bash
cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## ⚙️ 配置文件详解

构建后，所有 `config.json` 文件会自动复制到可执行文件目录。这些文件控制应用程序的行为。

### 1. `OmniNav_config.json`
```json
{
  "modules": [
    "Welcome to OmniNavigator",
    "DataManager",
    "OpticalNavigation",
    "ElectromagneticNavigation",
    "PunctureRobot"
  ]
}
```
- 定义主窗口加载的模块列表
- 添加或删除模块名以启用/禁用对应功能

### 2. `Base_config.json`
```json
{
  "display": {
    "view_modes": {
      "ALLWIN": 0,
      "AXIAL": 1,
      "VIEW3D": 2,
      "SAGITA": 3,
      "CORNAL": 4
    },
    "view_order": [1, 0, 2, 3],
    "colors": {
      "initial_2d_color": [0, 0, 0],
      "bottom_3d_color": [1.0, 1.0, 1.0],
      "top_3d_color": [0.3019, 0.3019, 0.4509]
    },
    "geometry": {
      "cross_line_length": 800,
      "line_actor_z_offset": 0,
      "units": { "cubic": "³" }
    },
    "camera": {
      "parallel_scale": 120,
      "zoom": 1.2,
      "distance": 300,
      "view_up": [
        [0, -1, 0],
        null,
        [0, 1, 0],
        [0, 1, 0]
      ]
    }
  }
}
```
- **view_modes**：视图枚举值，用于切换布局
- **view_order**：四个视图的显示顺序（[Axial, 3D, Coronal, Sagittal]）
- **colors**：2D 视图背景色、3D 视图上下背景色
- **geometry**：十字线长度、Z 偏移、单位符号
- **camera**：平行投影缩放、3D 视图相机参数

### 3. `DataManager_config.json`
```json
{
  "DataManager": {
    "range": { "minimum": -2000, "maximum": 2000 },
    "thresholds": {
      "lower_2d_value": -800,
      "upper_2d_value": 500,
      "lower_3d_value": -600,
      "upper_3d_value": 2000
    },
    "volume_property": {
      "colors": [[1.0,1.0,1.0],[1.0,1.0,1.0],[1.0,1.0,1.0]],
      "opacity": 1.0
    },
    "images": {
      "matrices": {
        "axial": [-1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
        "sagittal": [0,0,1,0, 1,0,0,0, 0,0,1,0, 0,0,0,1],
        "cornal": [1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,0,1]
      },
      "normals": {
        "axial": [0,0,1],
        "sagittal": [1,0,0],
        "cornal": [0,1,0]
      },
      "camera": { "parallel_scale": 120, "zoom": 1.2, "distance": 300, "view_up": [[0,-1,0],null,[0,1,0],[0,1,0]] },
      "scroll_list": [0,1,2]
    },
    "landmarks": { "color": [1,0,0], "radius": 2, "visible": 1, "opacity": 1.0 },
    "meshes": {
      "colors": [[1.0,0.9,0.8],[0.4,0.7,0.8],[0,1,0],[0,0,1],[1,1,0],[0,1,1]],
      "visible": 1, "opacity": 1.0
    },
    "tool": {
      "color": [1,1,0],
      "visible": 1,
      "opacity": 1.0,
      "matrix": [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]
    },
    "table_widget": { "text_margin": 1.2, "min_margin": 60 }
  }
}
```
- **range/thresholds**：影像窗宽窗位默认值
- **volume_property**：3D 体绘制颜色传输函数（3个控制点）和整体不透明度
- **images/matrices**：三个标准视图的坐标变换矩阵（从图像空间到世界空间）
- **images/normals**：各视图的平面法向量（用于切片提取）
- **landmarks/meshes/tool**：各对象的默认颜色、大小、可见性、变换矩阵
- **table_widget**：表格 UI 的边距设置

### 4. `OpticalNavigation_config.json`
```json
{
  "Device_IP": "192.168.1.202"
}
```
- **Device_IP**：NDI 跟踪系统的 IP 地址（默认 192.168.1.202）

### 5. `PunctureRobot_config.json`
```json
{
  "Device_IP": "192.168.1.15",
  "EndEffectorSerial": "590422944",
  "flange2endeffector": "1, 0, 0, 0, 0, 1, 0, 0.1875, 0, 0, 1, 0.1475, 0, 0, 0, 1"
}
```
- **Device_IP**：UR 机器人的 IP 地址
- **EndEffectorSerial**：末端执行器（步进电机）的序列号（用于 USB 识别）
- **flange2endeffector**： flange 到 EEF（针尖）的 4x4 齐次变换矩阵（行主序）

### 6. `ElectromagneticNavigation_config.json` & `BaseModule_config.json`
当前为空，预留用于未来的电磁导航模块。

---

## 🚀 快速入门

### 1. 首次运行
```powershell
# 进入构建输出目录
cd build/Release

# 运行程序（确保配置文件已复制）
.\OmniNav.exe
```

### 2. 导入医学影像
1. 点击菜单 `File → Open DICOM Folder`（或 `Open NIfTI`）
2. 选择 DICOM 文件夹或 `.nii/.nii.gz` 文件
3. 影像自动加载到 DataManager，四视图显示

### 3. 导航浏览
- **滚动**：鼠标滚轮或右侧滚动条
- **十字线**：在任意视图点击，其他视图自动同步到对应切片
- **窗宽窗位**：右键拖拽或使用工具栏滑块

### 4. 导入 3D 模型（如骨骼、器官）
1. `File → Import Mesh`
2. 选择 `.stl`/`.obj`/`.ply` 文件
3. 在 3D 视图显示，可调整透明度、颜色

### 5. 配置 NDI 光学跟踪
1. 编辑 `OpticalNavigation_config.json`，设置正确的 `Device_IP`
2. 启动 NDI 跟踪系统，确保与 PC 网络连通
3. 在 `OpticalNavigation` 模块点击 `Connect`
4. 加载工具 ROM 文件（`.rom`）
5. 工具显示在 3D 视图，实时更新位姿

### 6. 工具注册（Register）
1. 在影像上放置 fiducial marker（或使用已知 landmark）
2. 用 NDI 工具采样该点（`Sample` 按钮，多次采样取平均）
3. 点击 `Register` 计算工具到影像的变换
4. 注册精度可通过 landmarks 验证

### 7. 工具校准（Calibrate）
1. 将穿刺针固定在已知位置（如校准架）
2. 采样针尖位置（多个方向）
3. 点击 `Calibrate` 计算 tool-to-tip 变换
4. 校准数据保存到 `DataManager`

### 8. 连接机器人
1. 编辑 `PunctureRobot_config.json`：
   - `Device_IP`：UR 机器人 IP
   - `EndEffectorSerial`：步进电机序列号
   - `flange2endeffector`： flange 到针尖的变换矩阵
2. 在 `PunctureRobot` 模块点击 `Connect`
3. UR 机器人进入远程控制模式
4. 步进电机初始化

### 9. 执行穿刺工作流
1. 在 DataManager 选择目标点（通过 3D 视图或坐标输入）
2. 在 PunctureRobot 模块设置进给速度、深度
3. 点击 `Move to Target`：机器人移动到穿刺位姿
4. 点击 `Start Insertion`：步进电机控制针进给
5. 实时监控影像与跟踪数据

---

## 📂 项目结构

```
OmniNav/
├── CMakeLists.txt                 # 主构建文件
├── .gitignore                     # Git 忽略规则
├── README.md                      # 本文档
├── SRC/                           # 源代码根目录
│   ├── Base/                      # 主窗口基类与视图管理
│   │   ├── Base.h/cpp             # 主窗口类，菜单/模块加载
│   │   ├── Display.h/cpp          # 四视图显示管理
│   │   └── config.json            # 视图布局与外观配置
│   ├── Modules/                   # 功能模块
│   │   ├── BaseModule/            # 模块基类
│   │   ├── DataManager/           # 数据管理（影像、模型、标定）
│   │   ├── OpticalNavigation/     # NDI 光学导航
│   │   ├── ElectromagneticNavigation/  # 电磁导航（预留）
│   │   └── PunctureRobot/         # 机器人穿刺控制
│   │       ├── drivers/           # 底层驱动
│   │       │   ├── libs/windows/x86_64/librockmong.lib
│   │       │   ├── libs/windows/x86_64/rockmong.dll
│   │       │   └── src/           # 步进电机驱动源码
│   │       └── config.json        # 机器人 IP、EEF 配置
│   ├── Item/                      # 数据模型类
│   │   ├── Image.h/cpp            # 医学影像（ITK/VTK）
│   │   ├── Model3D.h/cpp          # 三维网格模型
│   │   ├── Landmark.h/cpp         # 兴趣点
│   │   ├── Tool.h/cpp             # NDI 工具定义
│   │   └── OmniTransform.h/cpp    # 坐标变换
│   ├── IO/                        # I/O 与线程
│   │   ├── NDIWorker.h/cpp        # NDI 跟踪线程
│   │   ├── URWorker.h/cpp         # UR 机器人线程
│   │   └── StepperWorker.h/cpp    # 步进电机线程
│   ├── CombinedAPI/               # NDI Combined API 封装
│   │   ├── CombinedApi.h/cpp      # 主 API 类
│   │   ├── TcpConnection.h/cpp    # TCP 连接
│   │   ├── ToolData.h/cpp         # 工具数据结构
│   │   └── ...                    # 其他数据结构
│   ├── CombinedAPIsample/         # NDI 官方示例库（子模块）
│   │   ├── library/include/       # 头文件
│   │   └── library/src/           # 源码
│   ├── ndicapi/                   # NDI C API（第三方库）
│   │   ├── ndicapi.cxx
│   │   ├── ndicapi_math.cxx
│   │   ├── ndicapi_serial_win32.cxx
│   │   └── ndicapi_socket_win32.cxx
│   ├── NewUrAPI/                  # Universal Robots RTDE API
│   │   ├── URController.h/cpp     # UR 控制器类
│   │   ├── RTDEInterface.h/cpp    # RTDE 通信接口
│   │   └── ...
│   ├── Utils/                     # 工具函数
│   │   ├── GeometryUtils.h/cpp    # 几何运算（矩阵、向量）
│   │   ├── VTKUtils.h/cpp         # VTK 辅助函数
│   │   └── ...
│   └── bin/                       # 构建输出（自动生成，不提交）
├── build/                         # 构建目录（不提交）
├── out/                           # 其他输出（不提交）
└── docs/                          # 文档（可选）
```

---

## 🔬 模块详解

### DataManager 模块
- **职责**：集中管理所有数据对象，提供统一的数据共享与持久化
- **核心类**：
  - `DataManager`：单例，管理所有 Image、Model3D、Landmark、Tool、OmniTransform
  - `Image`：封装 ITK 影像数据，提供 VTK 显示管线
  - `Model3D`：封装 vtkPolyData，支持 STL/OBJ/PLY 导入
  - `Landmark`：3D 点，可关联到影像坐标或世界坐标
  - `Tool`：NDI 工具定义（ROM 文件 + 实时位姿）
  - `OmniTransform`：坐标变换（4x4 矩阵）
- **信号槽**：数据变化时发射信号，通知 UI 更新

### OpticalNavigation 模块
- **职责**：NDI 光学跟踪系统的连接、数据采集、工具管理
- **核心类**：
  - `OpticalNavigation`：主模块类，UI 界面
  - `NDIWorker`：QThread，运行 CombinedApi 循环
  - `CombinedApi`：NDI Combined API 封装，处理 TCP 通信、数据解析
- **工作流**：
  1. `Connect` → 建立 TCP 连接，初始化设备
  2. `Load ROM` → 读取 `.rom` 文件，加载工具定义
  3. `Track` → 实时获取 6D 位姿，更新 Tool 对象
  4. `Sample` → 采集多帧，计算平均位姿（降低噪声）
  5. `Register` → 使用 sampled points 计算 tool-to-image 变换
  6. `Calibrate` → 计算 tool-to-tip 变换（针尖偏移）

### PunctureRobot 模块
- **职责**：UR 机器人与步进电机的联合控制，实现穿刺工作流
- **核心类**：
  - `PunctureRobot`：主模块类
  - `URWorker`：QThread，运行 RTDE 通信
  - `URController`：UR RTDE 客户端，发送关节/位姿指令，接收状态
  - `StepperController`：步进电机 USB 驱动（librockmong）
- **工作流**：
  1. `Connect` → 连接 UR（RTDE）和步进电机（USB）
  2. `Set Tool` → 加载 flange-to-EEF 变换，计算针尖位姿
  3. `Move to Pre-entry` → 机器人移动到穿刺起始点上方
  4. `Start Insertion` → 步进电机控制针以恒定速度进给
  5. `Monitor` → 实时显示针位姿、力反馈（如支持）
  6. `Stop` → 停止进给，机器人退针

### ElectromagneticNavigation 模块
- **状态**：预留，当前为空
- **规划**：集成电磁跟踪系统（如 Ascension 3DG），兼容 NDI 接口

---

## 💾 数据格式支持

| 格式 | 扩展名 | 读写 | 说明 |
|------|--------|-----|------|
| **DICOM** | `.dcm` 或文件夹 | ✅ 读 | 使用 ITK GDCM，支持多序列 |
| **NIfTI** | `.nii`, `.nii.gz` | ✅ 读 | 神经影像常用 |
| **NRRD** | `.nrrd` | ✅ 读 | 简单 raster 格式 |
| **MetaImage** | `.mhd`, `.mha` | ✅ 读 | ITK 原生格式 |
| **STL** | `.stl` | ✅ 读/写 | 三角网格（ASCII/Binary） |
| **OBJ** | `.obj` | ✅ 读 | Wavefront 格式（支持材质） |
| **PLY** | `.ply` | ✅ 读 | 多边形文件格式 |
| **VTK** | `.vtk` | ✅ 读 | VTK 原生格式 |
| **VTP** | `.vtp` | ✅ 读 | VTK XML 多边形数据 |
| **G** | `.g` | ✅ 读 | Geomview 格式 |
| **ROM** | `.rom` | ✅ 读 | NDI 工具定义（Marker 布局） |
| **JSON** | `.json` | ✅ 读/写 | landmarks、transforms 导出 |
| **TXT** | `.txt` | ✅ 读 | 信息面板文本显示 |

---

## 🛠️ 开发指南

### 添加新模块
1. 在 `SRC/Modules/` 创建新目录 `MyNewModule`
2. 创建 `MyNewModule.h/cpp`，继承 `BaseModule`
3. 实现 `init()`、`tick()`、`cleanup()` 虚函数
4. 在 `SRC/Modules/MyNewModule/config.json` 添加配置
5. 在 `CMakeLists.txt` 添加源文件列表
6. 在 `OmniNav_config.json` 的 `"modules"` 中添加 `"MyNewModule"`

### 调试技巧
- **日志**：使用 `spdlog::info()/debug()/error()`，日志输出到控制台和文件
- **VTK 调试**：设置 `VTK_DEBUG_LEAKS` 环境变量检测内存泄漏
- **Qt 调试**：使用 `QMessageBox` 或 `qDebug()` 输出
- **NDI 模拟**：使用 NDI 提供的模拟器（Simulator）测试连接
- **UR 模拟**：UR 机器人支持仿真模式（Virtual Mode），无需真实硬件

### 代码风格
- 缩进：4 空格
- 命名：类 `CamelCase`，函数/变量 `snake_case`，常量 `UPPER_SNAKE`
- 头文件保护：`#pragma once`（推荐）或 `#ifndef FILENAME_H`
- 指针：优先使用 `std::unique_ptr`，其次 `QSharedPointer`

---

## 🐛 故障排除

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| **CMake 找不到 Qt/VTK/ITK** | vcpkg 未集成或路径错误 | 运行 `vcpkg integrate install`，检查 `CMAKE_TOOLCHAIN_FILE` |
| **链接错误：undefined reference** | 库未链接或顺序错误 | 检查 `CMakeLists.txt` 中的 `target_link_libraries` |
| **运行时 DLL 缺失** | 构建时未拷贝 DLL | 确保 `vcpkg` 的 `bin/` 在 PATH 或手动拷贝 |
| **NDI 连接失败** | IP 错误、防火墙、设备离线 | 检查 `OpticalNavigation_config.json`，ping 设备 IP |
| **UR 连接超时** | IP 错误、RTDE 未启用 | 在 UR 示教器启用 RTDE，检查 `PunctureRobot_config.json` |
| **影像不显示** | DICOM 损坏、ITK 无法读取 | 尝试 NIfTI 格式，检查文件完整性 |
| **3D 视图黑屏** | GPU 驱动旧、OpenGL 版本低 | 更新显卡驱动，确保支持 OpenGL 3.2+ |
| **十字线不同步** | 影像方向矩阵错误 | 检查 `DataManager_config.json` 中的 `matrices` |
| **步进电机不响应** | USB 驱动未安装、序列号错误 | 安装 librockmong 驱动，检查 `EndEffectorSerial` |

---

## ✅ 测试与验证

### 单元测试（建议）
```bash
# 使用 Google Test 或 Catch2
mkdir build-test && cd build-test
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
cmake --build . --config Debug
ctest -C Debug
```

### 集成测试
1. **数据加载测试**：加载 DICOM、NIfTI、STL，检查渲染
2. **NDI 模拟测试**：使用 NDI Simulator，验证跟踪稳定性
3. **UR 仿真测试**：UR 机器人 Virtual Mode，验证运动规划
4. **穿刺工作流测试**：完整流程，从注册到执行

### 性能基准
- **帧率**：3D 视图 ≥ 30 FPS（带模型）
- **延迟**：NDI 数据到 UI 更新 ≤ 50 ms
- **内存**：加载 512x512x300 CT 影像 ≤ 2 GB

---

## 🤝 贡献指南

欢迎贡献！请遵循以下流程：

1. **Fork 本仓库**
2. **创建特性分支**：
   ```bash
   git checkout -b feature/AmazingFeature
   ```
3. **提交更改**：
   ```bash
   git commit -m "Add AmazingFeature"
   ```
4. **推送到分支**：
   ```bash
   git push origin feature/AmazingFeature
   ```
5. **开启 Pull Request**，描述：
   - 解决了什么问题
   - 如何测试
   - 是否有 breaking changes

### 代码规范
- 遵循现有代码风格
- 添加新文件时更新 `CMakeLists.txt`
- 修改配置时更新文档
- 提交前运行 `git status` 确认无多余文件

---

## 📄 许可证

**本项目目前未包含 LICENSE 文件。**

建议添加 MIT 许可证以促进开源协作：
```bash
# 下载 MIT 模板
curl -o LICENSE https://opensource.org/license/mit
git add LICENSE
git commit -m "Add MIT License"
```

---

## 📚 参考文献

- **VTK 官方文档**：https://vtk.org/doc/
- **ITK 官方文档**：https://itk.org/ITKSoftwareGuide/html/
- **Qt 官方文档**：https://doc.qt.io/
- **NDI Combined API 指南**：https://www.ndigital.com/developers/
- **Universal Robots RTDE 手册**：https://www.universal-robots.com/download/
- **DICOM 标准**：https://www.dicomstandard.org/

---

## 📧 联系方式

- **作者**：Haitao-Lee
- **GitHub Issues**：https://github.com/Haitao-Lee/OmniNav/issues
- **邮件**：（请添加联系方式）

---

## 🎉 致谢

- NDI（Northern Digital Inc.）提供 Combined API 支持
- Universal Robots 提供 RTDE 协议文档
- VTK/ITK 开源社区
- Qt 框架

---

**最后更新时间**：2026-03-14  
**文档版本**：1.0.0  
**项目版本**：0.1.0

---

*OmniNav — 让医疗影像导航更精准、更智能*