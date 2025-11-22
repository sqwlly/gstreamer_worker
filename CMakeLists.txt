cmake_minimum_required(VERSION 3.21)
project(gstreamer_worker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)

option(ENABLE_NVENC "Enable NVIDIA specific plugins" ON)
option(ENABLE_FEC "Enable optional FEC elements" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
    gstreamer-rtp-1.0
    gstreamer-rtsp-1.0
    gstreamer-sdp-1.0
    gstreamer-pbutils-1.0
    gstreamer-allocators-1.0
)

add_library(gstreamer_worker::gst INTERFACE IMPORTED)
target_link_libraries(gstreamer_worker::gst INTERFACE
    PkgConfig::GST
)

add_library(project_options INTERFACE)
add_library(gstreamer_worker::project_options ALIAS project_options)

target_compile_features(project_options INTERFACE cxx_std_20)

target_compile_options(project_options INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive- /W4 /MP>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_subdirectory(libs/zerocopy)
add_subdirectory(libs/pipeline)
add_subdirectory(libs/control)
add_subdirectory(apps/capture_server)
add_subdirectory(apps/viewer_client)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
