cmake_minimum_required(VERSION 3.28)

cmake_policy(SET CMP0167 NEW)

# 指定 vcpkg 工具链
set(CMAKE_TOOLCHAIN_FILE "../../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(OmniNav VERSION 0.1.0 LANGUAGES CXX)

add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 基础库查找
find_package(Qt5 REQUIRED COMPONENTS Widgets Network Core Gui SerialPort)
find_package(spdlog REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# ==================== VTK 配置 ====================
find_package(VTK REQUIRED)
set(PROJECT_VTK_LIBS "")

if (VTK_MAJOR_VERSION GREATER_EQUAL 9)
    message(STATUS "VTK 9 detected. Using modern targets.")
    find_package(VTK REQUIRED COMPONENTS 
        CommonCore 
        RenderingCore 
        RenderingOpenGL2 
        GUISupportQt
        ImagingCore
        IOImage
    )
    list(APPEND PROJECT_VTK_LIBS
        VTK::CommonCore 
        VTK::RenderingCore 
        VTK::RenderingOpenGL2 
        VTK::GUISupportQt
        VTK::ImagingCore
        VTK::IOImage
    )
else()
    message(STATUS "VTK 8 detected. Using standard libraries.")
    include(${VTK_USE_FILE})
    list(APPEND PROJECT_VTK_LIBS ${VTK_LIBRARIES})
endif()

# unset(VTK_DIR CACHE)
# unset(VTK_LIBRARIES CACHE)
# unset(VTK_INCLUDE_DIRS CACHE)
# unset(VTK_VERSION CACHE)

# find_package(VTK 9.0 REQUIRED COMPONENTS  # 需大于9.0
#     CommonCore
#     RenderingCore
#     RenderingOpenGL2
#     GUISupportQt
#     ImagingCore
#     IOImage
# )

# message(STATUS "VTK Version: ${VTK_VERSION}")
# message(STATUS "VTK Include dirs: ${VTK_INCLUDE_DIRS}")

# set(PROJECT_VTK_LIBS
#     VTK::CommonCore
#     VTK::RenderingCore
#     VTK::RenderingOpenGL2
#     VTK::GUISupportQt
#     VTK::ImagingCore
#     VTK::IOImage
# )

# ==================== ITK 配置 (新增) ====================
find_package(ITK REQUIRED COMPONENTS
    ITKCommon            # 核心
    ITKIOImageBase       # IO 基础
    ITKIOGDCM            # DICOM 支持
    ITKIONIFTI           # NIfTI 支持
    ITKImageIntensity    # 包含 RescaleIntensity 等图像处理过滤器
)
include(${ITK_USE_FILE})

# ==================== 配置文件与源码处理 ====================
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi/ndicapiExport.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ndicapiExport.h"
)

file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/*.ui")
file(GLOB_RECURSE ALL_RESOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/*.qrc")

# 递归或分组获取所有源码
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/*.cpp")
file(GLOB HEAD_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/*.h")

# 各子模块源码获取 
file(GLOB BASE_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Base/*.cpp")
file(GLOB BASE_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Base/*.h")
file(GLOB ITEM_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Item/*.cpp")
file(GLOB ITEM_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Item/*.h")
file(GLOB IO_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/IO/*.cpp")
file(GLOB IO_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/IO/*.h")
file(GLOB COMBINEDAPI_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/CombinedAPI/*.cpp")
file(GLOB COMBINEDAPI_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/CombinedAPI/*.h")
file(GLOB BASEMODULE_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/BaseModule/*.cpp")
file(GLOB BASEMODULE_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/BaseModule/*.h")
file(GLOB DATAMANAGER_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/DataManager/*.cpp")
file(GLOB DATAMANAGER_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/DataManager/*.h")
file(GLOB OPTICALNAVIGATION_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/OpticalNavigation/*.cpp")
file(GLOB OPTICALNAVIGATION_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/OpticalNavigation/*.h")
file(GLOB ELECTROMAGNETICNAVIGATION_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/ElectromagneticNavigation/*.cpp")
file(GLOB ELECTROMAGNETICNAVIGATION_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/ElectromagneticNavigation/*.h")
file(GLOB PUNCROBOT_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/*.cpp")
file(GLOB PUNCROBOT_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/*.h")
file(GLOB PUNCROBOT_DRIVERS_SRC CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/drivers/*.c" 
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/drivers/*.cpp"
)
file(GLOB PUNCROBOT_DRIVERS_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/drivers/*.h")

set(NDICAPI_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi/ndicapi.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi/ndicapi_math.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi/ndicapi_serial_win32.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi/ndicapi_socket_win32.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi/ndicapi_thread.cxx"
)

file(GLOB NDICAPI_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi/*.h")

file(GLOB UR_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/NewUrAPI/*.cpp")
file(GLOB UR_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/NewUrAPI/*.h")
file(GLOB UR_HPP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/NewUrAPI/*.hpp")
file(GLOB UTIL_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Utils/*.cpp")
file(GLOB UTIL_HDR CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Utils/*.h")

# ==================== 可执行文件构建 ====================
add_executable(${PROJECT_NAME}
    ${HEAD_FILES} ${SRC_FILES}
    ${BASE_HDR} ${BASE_SRC}
    ${IO_HDR} ${IO_SRC}
    ${ITEM_HDR} ${ITEM_SRC}
    ${BASEMODULE_HDR} ${BASEMODULE_SRC}
    ${DATAMANAGER_HDR} ${DATAMANAGER_SRC}
    ${OPTICALNAVIGATION_HDR} ${OPTICALNAVIGATION_SRC}
    ${ELECTROMAGNETICNAVIGATION_HDR} ${ELECTROMAGNETICNAVIGATION_SRC}
    ${PUNCROBOT_HDR} ${PUNCROBOT_SRC}
    ${PUNCROBOT_DRIVERS_HDR} ${PUNCROBOT_DRIVERS_SRC}
    ${NDICAPI_HDR} ${NDICAPI_SRC}
    ${COMBINEDAPI_HDR} ${COMBINEDAPI_SRC}
    ${UR_HDR} ${UR_SRC} ${UR_HPP}
    ${UTIL_HDR} ${UTIL_SRC}
    ${UI_FILES} ${ALL_RESOURCES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ndicapi_EXPORTS
    TIMEOUT_PERIOD=5000
)



# 包含路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Base
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Item
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/IO
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/BaseModule
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/DataManager
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/OpticalNavigation
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/ElectromagneticNavigation
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/ndicapi
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/NewUrAPI
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/CombinedAPI
    ${CMAKE_CURRENT_SOURCE_DIR}/SRC/Utils
    ${CMAKE_CURRENT_BINARY_DIR}
)

# ==================== 链接库 ====================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::Core 
    Qt5::Gui 
    Qt5::Widgets 
    Qt5::SerialPort
    spdlog::spdlog
    Eigen3::Eigen
    Boost::system
    Boost::thread
    ${PROJECT_VTK_LIBS}
    ${ITK_LIBRARIES}     # 链接 ITK 库
    ws2_32

    "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/drivers/libs/windows/x86_64/librockmong.lib"
)


# ==================== 2. 自动拷贝动态库 (.dll) ====================
# 获取目录下所有的 .dll 文件
file(GLOB WINDOWS_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/SRC/Modules/PunctureRobot/drivers/libs/windows/x86_64/*.dll")

# 编译生成 .exe 后，自动把 .dll 拷贝过去
foreach(DLL_FILE ${WINDOWS_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_FILE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying DLL: ${DLL_FILE} to binary directory"
    )
endforeach()


# ==================== 资源拷贝 ====================
set(CONFIG_FILES
    "Modules/PunctureRobot/config.json:PunctureRobot_config.json"
    "Modules/DataManager/config.json:DataManager_config.json"
    "Modules/OpticalNavigation/config.json:OpticalNavigation_config.json"
    "Modules/ElectromagneticNavigation/config.json:ElectromagneticNavigation_config.json"
    "Modules/BaseModule/config.json:BaseModule_config.json"
    "Base/config.json:Base_config.json"
    "config.json:OmniNav_config.json"
)

foreach(FILE_MAP ${CONFIG_FILES})
    string(REPLACE ":" ";" MAP_LIST ${FILE_MAP})
    list(GET MAP_LIST 0 SRC_REL_PATH)
    list(GET MAP_LIST 1 DST_NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/SRC/${SRC_REL_PATH}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DST_NAME}"
    )
endforeach()

# ==================== IDE 分组 ====================
source_group("Source Files" FILES ${SRC_FILES})
source_group("Header Files" FILES ${HEAD_FILES})
source_group("Base" FILES ${BASE_SRC} ${BASE_HDR})
source_group("Item" FILES ${ITEM_SRC} ${ITEM_HDR})
source_group("IO" FILES ${IO_SRC} ${IO_HDR})
source_group("Modules\\BaseModule" FILES ${BASEMODULE_SRC} ${BASEMODULE_HDR})
source_group("Modules\\DataManager" FILES ${DATAMANAGER_SRC} ${DATAMANAGER_HDR})
source_group("Modules\\OpticalNavigation" FILES ${OPTICALNAVIGATION_SRC} ${OPTICALNAVIGATION_HDR})
source_group("Modules\\ElectromagneticNavigation" FILES ${ELECTROMAGNETICNAVIGATION_SRC} ${ELECTROMAGNETICNAVIGATION_HDR})
source_group("Modules\\PunctureRobot" FILES ${PUNCROBOT_SRC} ${PUNCROBOT_HDR})
source_group("Modules\\PunctureRobot\\drivers" FILES ${PUNCROBOT_DRIVERS_SRC} ${PUNCROBOT_DRIVERS_HDR})
source_group("NDICAPI" FILES ${NDICAPI_SRC} ${NDICAPI_HDR})
source_group("CombinedAPI" FILES ${COMBINEDAPI_SRC} ${COMBINEDAPI_HDR})
source_group("UrAPI" FILES ${UR_SRC} ${UR_HDR} ${UR_HPP})
source_group("Utils" FILES ${UTIL_SRC} ${UTIL_HDR})
source_group("Resources" FILES ${UI_FILES} ${ALL_RESOURCES})