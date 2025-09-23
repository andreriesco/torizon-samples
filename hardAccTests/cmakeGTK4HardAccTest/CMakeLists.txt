cmake_minimum_required(VERSION 3.0.0)

project(cmakeGTK4HATest VERSION 1.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# Find OpenGL (optional)
find_package(OpenGL)

# Find EGL (optional)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(EGL egl)
endif()

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
include_directories(${GTK4_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/includes)
link_directories(${GTK4_LIBRARY_DIRS})

# Add other flags to the compiler
add_definitions(${GTK4_CFLAGS_OTHER})

# Add sources
set(SOURCES
    src/main.cpp
    src/HardwareAccelerationService.cpp
)

add_executable(cmakeGTK4HATest ${SOURCES})

# Link libraries
target_link_libraries(cmakeGTK4HATest ${GTK4_LIBRARIES})

# Link OpenGL if available
if(OpenGL_FOUND)
    target_link_libraries(cmakeGTK4HATest ${OPENGL_LIBRARIES})
    target_include_directories(cmakeGTK4HATest PRIVATE ${OPENGL_INCLUDE_DIRS})
endif()

# Link EGL if available
if(EGL_FOUND)
    target_link_libraries(cmakeGTK4HATest ${EGL_LIBRARIES})
    target_include_directories(cmakeGTK4HATest PRIVATE ${EGL_INCLUDE_DIRS})
endif()

# Link filesystem library for C++17
target_link_libraries(cmakeGTK4HATest stdc++fs)

# Link pthread for threading support
find_package(Threads REQUIRED)
target_link_libraries(cmakeGTK4HATest Threads::Threads)

set(CMAKE_SYSTEM_NAME Linux)

set_target_properties(cmakeGTK4HATest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
    ARCHIVE_OUTPUT_DIRECTORY "lib"
    LIBRARY_OUTPUT_DIRECTORY "lib"
)
