cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0099 NEW)

project(Style VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR})

find_package(Qt6 6.2.4 COMPONENTS Quick REQUIRED)

qt_add_library(Style STATIC)
qt_add_qml_module(Style
    URI Style
    VERSION 1.0
    QML_FILES Style.qml
)

set_target_properties(Style PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(Style
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(Style
    PRIVATE Qt6::Quick)

target_include_directories(Style PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
