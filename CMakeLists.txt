cmake_minimum_required(VERSION 3.16)
project(globalProject)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Charts PrintSupport OpenGL)

add_subdirectory(src)