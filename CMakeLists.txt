cmake_minimum_required(VERSION 3.16)
option(VITACAM_HOST "Build desktop checks and UI snapshots" OFF)
if(NOT VITACAM_HOST)
  if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
  endif()
endif()
project(vita_cam VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)
add_library(camera_core src/core.cpp src/export.cpp src/ui.cpp src/capture_writer.cpp)
target_include_directories(camera_core PUBLIC include)
if(VITACAM_HOST)
  find_package(PNG REQUIRED)
  find_package(Threads REQUIRED)
  target_link_libraries(camera_core PUBLIC PNG::PNG Threads::Threads)
  enable_testing()
  add_executable(core_tests tests/core_tests.cpp)
  target_link_libraries(core_tests PRIVATE camera_core)
  add_test(NAME capture_pipeline COMMAND core_tests)
  add_executable(writer_tests tests/writer_tests.cpp)
  target_link_libraries(writer_tests PRIVATE camera_core)
  add_test(NAME background_saving COMMAND writer_tests)
  add_executable(ui_snapshot tests/ui_snapshot.cpp)
  target_link_libraries(ui_snapshot PRIVATE camera_core)
else()
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
  add_executable(vita_cam src/main.cpp src/camera.cpp src/storage.cpp src/vita_painter.cpp)
  target_compile_options(camera_core PRIVATE -O2)
  target_compile_options(vita_cam PRIVATE -O2)
  target_link_libraries(vita_cam PRIVATE camera_core vita2d png jpeg z m pthread
    SceCamera_stub SceDisplay_stub SceGxm_stub SceSysmodule_stub SceCtrl_stub
    SceTouch_stub ScePgf_stub ScePvf_stub SceCommonDialog_stub ScePower_stub
    SceAppMgr_stub SceIofilemgr_stub SceRtc_stub SceKernelThreadMgr_stub)
  vita_create_self(eboot.bin vita_cam UNSAFE)
  vita_create_vpk(vita-cam.vpk VITACAM01 eboot.bin
    VERSION 01.00 NAME "Vita Camera"
    FILE assets/icon0.png sce_sys/icon0.png
         assets/livearea/bg.png sce_sys/livearea/contents/bg.png
         assets/livearea/startup.png sce_sys/livearea/contents/startup.png
         assets/livearea/template.xml sce_sys/livearea/contents/template.xml)
endif()
