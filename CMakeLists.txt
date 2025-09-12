cmake_minimum_required(VERSION 3.16)
project(Qt-Hyprpaper-GUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

# Include all source files (main.cpp + others)
set(SOURCES
    src/main.cpp
    src/reload.cpp
    src/gpu_renderer.cpp
    src/inotifywatcher.cpp
)
set(HEADERS
    src/reload.h
    src/cachedImage.h
    src/paths.h
    gpu_renderer.h
    src/inotifywatcher.h     
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

# Enable automatic MOC, UIC, and RCC
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# For safety, ensure C++17 is used
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
