cmake_minimum_required(VERSION 3.10)
project(lvgl_hardaccel_test C)

# Uncomment if the program needs debugging
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

set(CMAKE_C_STANDARD 99) # LVGL officially supports C99 and above
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Use the package PkgConfig to find OpenGL and EGL (optional)
find_package(PkgConfig)

# Find OpenGL (optional)
find_package(OpenGL)

# Find EGL (optional)
if(PkgConfig_FOUND)
    pkg_check_modules(EGL egl)
endif()

add_subdirectory(lib/lvgl)

target_include_directories(lvgl PUBLIC lib/ src/ includes/)

# Add sources
set(SOURCES
    src/main.c
    src/hardware_acceleration_service.c
)

add_executable(main ${SOURCES})

target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg m)

# Link OpenGL if available
if(OpenGL_FOUND)
    target_link_libraries(main ${OPENGL_LIBRARIES})
    target_include_directories(main PRIVATE ${OPENGL_INCLUDE_DIRS})
endif()

# Link EGL if available
if(EGL_FOUND)
    target_link_libraries(main ${EGL_LIBRARIES})
    target_include_directories(main PRIVATE ${EGL_INCLUDE_DIRS})
endif()

# Link pthread for threading support
find_package(Threads REQUIRED)
target_link_libraries(main Threads::Threads)

add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)


