cmake_minimum_required(VERSION 3.16)
project(cppQMLHATest LANGUAGES CXX C)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Concurrent)

qt_add_executable(cppQMLHATest WIN32
    main.cpp
    HardwareAccelerationService.h
    HardwareAccelerationService.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(cppQMLHATest PRIVATE QT_QML_DEBUG)
endif()

set_target_properties(cppQMLHATest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
    ARCHIVE_OUTPUT_DIRECTORY "lib"
    LIBRARY_OUTPUT_DIRECTORY "lib"
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(cppQMLHATest PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::Concurrent
)

qt_add_qml_module(cppQMLHATest
    URI cppQMLHATest
    VERSION 1.0
    QML_FILES
        "QML/main.qml"
        "QML/Scene.ui.qml"
        "QML/HardwareAccelerationScene.qml"
    RESOURCES
        "QML/assets/torizon-logo.png"
)


