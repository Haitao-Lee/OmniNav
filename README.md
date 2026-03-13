# OmniNav

OmniNav is a Qt-based medical imaging navigation application that integrates multi-view VTK visualization, optical tracking (NDI Combined API), and a robot puncture workflow (Universal Robots + stepper end-effector). The UI is modular and supports image/mesh import, landmark and tool management, registration, calibration, and real-time tracking.

## Table of Contents
- [Features](#features)
- [Architecture](#architecture)
- [Tech Stack](#tech-stack)
- [Build and Run](#build-and-run)
- [Configuration](#configuration)
- [Data Formats](#data-formats)
- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Hardware Notes](#hardware-notes)
- [License](#license)

## Features
- Four synchronized views: Axial, 3D, Coronal, Sagittal
- Crosshair interaction and slice synchronization
- Image import/export: DICOM (folder or .dcm), NIfTI (.nii/.nii.gz), NRRD, MHD/MHA
- 3D model import/export: STL, OBJ, PLY, VTK, VTP, G
- Landmark, tool, and transform management via tables
- Optical navigation: connect, track, sample, register, calibrate
- Robot puncture workflow with UR robot control and stepper needle drive
- JSON-based configuration copied next to the executable at build time

## Architecture
```mermaid
flowchart LR
    UI[OmniNav] --> DM[DataManager]
    UI --> ON[OpticalNavigation]
    UI --> EM[ElectromagneticNavigation]
    UI --> PR[PunctureRobot]

    DM --> Items[Image / Model3D / Landmark / Tool / OmniTransform]
    DM --> VTK[VTK Rendering + ITK/VTK IO]

    ON --> NDIW[NDICombinedWorker (QThread)]
    NDIW --> CAPI[CombinedApi / ndicapi]

    PR --> RW[RobotWorker (QThread)]
    RW --> UR[URController (RTDE)]
    RW --> STEP[Stepper Driver (usb_device/pwm)]
```

## Tech Stack
- C++20
- Qt 5 (Widgets, Network, Core, Gui, SerialPort)
- VTK (RenderingOpenGL2, GUISupportQt, Imaging, IOImage)
- ITK (ITKCommon, ITKIOImageBase, ITKIOGDCM, ITKIONIFTI, ITKImageIntensity)
- Eigen3, Boost (system, thread), spdlog
- CMake 3.28+

## Build and Run
### Windows (Visual Studio 2022)
```bash
cmake -S . -B build \
  -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release
```

### Run
- The executable is generated under the build output directory, for example:
  - `build/Release/OmniNav.exe`
- Post-build steps copy DLLs and JSON configs into the executable folder.
- Launch the app from the output folder so it can load `*_config.json` files.

## Configuration
These JSON files live under `SRC/` and are copied next to the executable on build:
- `OmniNav_config.json`: module list
- `Base_config.json`: view colors, crosshair geometry, camera settings
- `DataManager_config.json`: thresholds, volume properties, table UI settings
- `OpticalNavigation_config.json`: NDI device IP
- `PunctureRobot_config.json`: robot IP, end-effector serial, flange-to-EEF transform
- `ElectromagneticNavigation_config.json`, `BaseModule_config.json`: currently empty defaults

## Data Formats
- Images: `.dcm` (file or directory), `.nii`, `.nii.gz`, `.nrrd`, `.mhd`, `.mha`
- 3D models: `.stl`, `.obj`, `.ply`, `.vtk`, `.vtp`, `.g`
- Tools: `.rom` (NDI tool definition)
- Landmarks/transforms: `.json` (exported/imported by app)
- Text: `.txt` imported to the info panel

## Quick Start
1. Build the project and run `OmniNav.exe` from the build output folder.
2. In DataManager, import a DICOM folder or image file.
3. Use scrollbars or crosshair clicks to navigate slices.
4. Import 3D models and adjust visibility/opacity.
5. Add tool ROMs and connect to the NDI system in OpticalNavigation.
6. Track tools, perform sampling/registration/calibration.
7. Connect robot and end-effector in PunctureRobot, then run the puncture workflow.

## Project Structure
```
SRC/
  Base/                 # Main window base class and VTK view setup
  Modules/              # DataManager, OpticalNavigation, PunctureRobot, ElectromagneticNavigation
  Item/                 # Image, Model3D, Landmark, Tool, OmniTransform
  IO/                   # NDICombinedWorker
  CombinedAPI/          # NDI Combined API wrapper
  ndicapi/              # NDI C API sources
  NewUrAPI/             # UR RTDE communication
  Utils/                # Geometry and VTK visualization helpers
bin/                    # Generated build output (recreatable)
CMakeLists.txt
```

## Hardware Notes
- OpticalNavigation expects an NDI tracking system reachable by IP.
- PunctureRobot expects a Universal Robots arm and a USB stepper driver.
- Validate safety and environment before using real hardware.

## License
No license file is included. Add one before publishing on GitHub.
