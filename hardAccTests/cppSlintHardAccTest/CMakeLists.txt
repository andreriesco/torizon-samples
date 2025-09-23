cmake_minimum_required(VERSION 3.21)
project(cppSlintHATest LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Slint QUIET)
if (NOT Slint_FOUND)
  message("Slint could not be located in the CMake module search path. Downloading it from Git and building it locally")
  include(FetchContent)
  FetchContent_Declare(
    Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    # `release/1` will auto-upgrade to the latest Slint >= 1.0.0 and < 2.0.0
    # `release/1.0` will auto-upgrade to the latest Slint >= 1.0.0 and < 1.1.0
    GIT_TAG release/1
    SOURCE_SUBDIR api/cpp
  )
  FetchContent_MakeAvailable(Slint)
endif (NOT Slint_FOUND)

# Enable referenced images into the binary by default, so that deployment into docker
# is just copying the application binary.
set(DEFAULT_SLINT_EMBED_RESOURCES "embed-files")

add_executable(cppSlintHATest 
    src/main.cpp
    src/HardwareAccelerationService.h
    src/HardwareAccelerationService.cpp
)

target_link_libraries(cppSlintHATest PRIVATE Slint::Slint)

# Try to find and link additional libraries for OpenGL and system detection
if(UNIX AND NOT APPLE)
    # Linux-specific libraries - make them optional
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(EGL egl QUIET)
        pkg_check_modules(GL gl QUIET)
        pkg_check_modules(X11 x11 QUIET)
        
        if(EGL_FOUND)
            target_link_libraries(cppSlintHATest PRIVATE ${EGL_LIBRARIES})
            target_include_directories(cppSlintHATest PRIVATE ${EGL_INCLUDE_DIRS})
            target_compile_definitions(cppSlintHATest PRIVATE HAS_EGL_PKG=1)
        endif()
        
        if(GL_FOUND)
            target_link_libraries(cppSlintHATest PRIVATE ${GL_LIBRARIES})
            target_include_directories(cppSlintHATest PRIVATE ${GL_INCLUDE_DIRS})
            target_compile_definitions(cppSlintHATest PRIVATE HAS_GL_PKG=1)
        endif()
        
        if(X11_FOUND)
            target_link_libraries(cppSlintHATest PRIVATE ${X11_LIBRARIES})
            target_include_directories(cppSlintHATest PRIVATE ${X11_INCLUDE_DIRS})
            target_compile_definitions(cppSlintHATest PRIVATE HAS_X11_PKG=1)
        endif()
    endif()
    
    # Standard system libraries (these should always be available)
    target_link_libraries(cppSlintHATest PRIVATE dl pthread)
elseif(WIN32)
    # Try to find OpenGL on Windows
    find_package(OpenGL QUIET)
    if(OpenGL_FOUND)
        target_link_libraries(cppSlintHATest PRIVATE OpenGL::GL)
    endif()
endif()

slint_target_sources(cppSlintHATest ui/appwindow.slint)

# On Windows, copy the Slint DLL next to the application binary so that it's found.
if (WIN32)
    add_custom_command(TARGET cppSlintHATest POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:cppSlintHATest> $<TARGET_FILE_DIR:cppSlintHATest> COMMAND_EXPAND_LISTS)
endif()
