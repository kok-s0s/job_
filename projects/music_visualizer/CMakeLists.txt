cmake_minimum_required(VERSION 3.21)
project(MusicVisualizer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt6 installed via Homebrew
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt6")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(MusicVisualizer
    src/main.cpp
    src/AudioAnalyzer.cpp
    src/AudioAnalyzer.h
)

qt_add_qml_module(MusicVisualizer
    URI MusicVisualizer
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
)

target_include_directories(MusicVisualizer PRIVATE src)

set_target_properties(MusicVisualizer PROPERTIES OUTPUT_NAME "music-visualizer")

target_link_libraries(MusicVisualizer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Quick
    Qt6::QuickControls2
    "-framework Accelerate"   # macOS built-in FFT via vDSP
)
