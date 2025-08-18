qt_add_library(RuntableLib STATIC)

set_target_properties(RuntableLib PROPERTIES AUTOMOC ON)

target_link_libraries(RuntableLib PRIVATE Qt6::Core Qt6::Qml)

# The QML module now links against the C++ library
qt_add_qml_module(RuntableLib
    URI com.grams.prototable
    VERSION 1.0
    RESOURCE_PREFIX /modules/RuntableLib
    QML_FILES
        RunTable.qml
        ConditionCell.qml
        ControlDelegate.qml
        RegimeDelegate.qml
        StateDelegate.qml
        TimeProgressBar.qml
        ScrollArrow.qml
    SOURCES
        prototablemodel.cpp
        prototablemodel.h
        regime.cpp
        regime.h
        regimemanager.cpp
        regimemanager.h
        visibleregimemodel.cpp
        visibleregimemodel.h
)

# Add the tests subdirectory
enable_testing()
add_subdirectory(tests)

