# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```powershell
# Create build directory and configure
mkdir build && cd build
cmake -S .. -B . -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake"

# Build Release
cmake --build . --config Release

# Build output: build/Release/OmniNav.exe
```

**Requirements**: Visual Studio 2022, vcpkg with Qt5, VTK 9+, ITK 5+, spdlog, Eigen3, Boost, NDI Combined API SDK.

## Architecture

OmniNav is a Qt5/Qt6 C++20 medical imaging navigation application with four synchronized VTK views (Axial, 3D, Coronal, Sagittal), NDI optical tracking integration, and a robotic puncture workflow.

### Module System

Each module inherits `BaseModule` (which inherits `QMainWindow`) and implements `init()`, `tick()`, and `cleanup()`. Modules are loaded dynamically via the `modules` array in `OmniNav_config.json`. All modules communicate via Qt signals/slots.

### Core Modules

- **Base** (`SRC/Base/`): Main window managing four VTK render windows, crosshair synchronization, camera control, and menu/toolbar. Holds `Display` objects for each view.
- **DataManager** (`SRC/Modules/DataManager/`): Central data hub. Manages `Image` (ITK/VTK), `Model3D` (vtkPolyData), `Landmark`, `Tool`, and `OmniTransform` objects. Emits signals when data changes.
- **OpticalNavigation** (`SRC/Modules/OpticalNavigation/`): NDI tracking via `NDICombinedWorker` (QThread). Handles tool ROM loading, 6D pose tracking, sampling, registration (point-to-point landmark matching), and pivot calibration.
- **PunctureRobot** (`SRC/Modules/PunctureRobot/`): UR robot control via `RobotWorker` (QThread) using RTDE protocol, plus stepper motor driver via `librockmong` USB driver. State machine: `PHASE_IDLE` → `PHASE_MOVING_TO_APPROACH` → `PHASE_MOVING_TO_ENTRY` → `PHASE_READY` → insertion → `PHASE_RETURNING_*`.

### Data Layer

- **Image** (`SRC/Item/Image.h`): Wraps ITK image with VTK pipeline for display (2D slices + 3D volume rendering via `vtkGPUVolumeRayCastMapper`).
- **Model3D** (`SRC/Item/Model3D.h`): Wraps `vtkPolyData` for STL/OBJ/PLY meshes.
- **Tool** (`SRC/Item/Tool.h`): NDI tool definition (ROM file + real-time pose).
- **Landmark** (`SRC/Item/Landmark.h`): 3D point with name and color.
- **OmniTransform** (`SRC/Item/OmniTransform.h`): 4x4 Eigen matrix for coordinate transforms.

### Hardware I/O Threads

- **NDICombinedWorker** (`SRC/IO/NDICombinedWorker.h/cpp`): QThread running CombinedApi loop — TCP connect, tool tracking, sampling. Emits `handleUpdated(TrackedTool)` with pose data.
- **RobotWorker** (`SRC/Modules/PunctureRobot/RobotWorker.h/cpp`): QThread for UR RTDE communication — sends joint/pose commands, receives robot status.

### External APIs

- **CombinedAPI** (`SRC/CombinedAPI/`): Wrapper around NDI's `ndicapi` (C API in `SRC/ndicapi/`). Handles GBF frame parsing, CRC checking, TCP/serial connections.
- **NewUrAPI** (`SRC/NewUrAPI/`): UR RTDE client — `URController` class manages connection and command sending.

### Configuration

JSON configs in `SRC/*/config.json` are auto-copied to the executable directory at build time. Key configs:
- `OmniNav_config.json`: module enable/disable list
- `Base_config.json`: view layout, colors, camera parameters
- `DataManager_config.json`: window/level defaults, volume rendering, table settings
- `OpticalNavigation_config.json`: NDI device IP
- `PunctureRobot_config.json`: UR robot IP, stepper serial, flange-to-EEF transform

### UI Files

Qt `.ui` files processed by `cmake AUTOUIC`. Corresponding `ui_*.h` headers are generated in `${CMAKE_BINARY_DIR}`.

## Adding a New Module

1. Create `SRC/Modules/MyNewModule/` with `MyNewModule.h/cpp`
2. Inherit `BaseModule` and implement `init()`, `initSplitters()`, `initButton()`, `createActions()`, `loadConfig()`
3. Add `config.json` to `SRC/Modules/MyNewModule/`
4. Add to CMakeLists `PUNCROBOT_SRC` glob (or create new group)
5. Add `"MyNewModule"` to `OmniNav_config.json` modules array
