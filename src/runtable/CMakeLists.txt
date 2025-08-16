cmake_minimum_required(VERSION 2.8...4.0)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Qml)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Create a static library for the C++ logic
add_library(prototablemodel STATIC
    prototablemodel.cpp
    regime.cpp
    regimemanager.cpp
    visibleregimemodel.cpp
)
set_target_properties(prototablemodel PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Public headers for the library
target_include_directories(prototablemodel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link the library to Qt
target_link_libraries(prototablemodel PRIVATE Qt6::Core Qt6::Qml)

# The QML module now links against the C++ library
qt_add_qml_module(runtable
    URI com.grams.prototable
    VERSION 1.0
    QML_FILES
        RunTable.qml
        ConditionCell.qml
        ControlDelegate.qml
        RegimeDelegate.qml
        StateDelegate.qml
        TimeProgressBar.qml
        ScrollArrow.qml
    SOURCES
        prototablemodel.h
        regime.h
        regimemanager.h
        visibleregimemodel.h
)

# Link the QML module to the logic library
target_link_libraries(runtable PRIVATE prototablemodel)

# Add the tests subdirectory
enable_testing()
add_subdirectory(tests)

