cmake_minimum_required(VERSION 3.21)
project(globalProject)
enable_testing()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Quick QuickControls2 Charts PrintSupport SerialPort OpenGL Sql Concurrent TaskTree)

add_subdirectory(src)