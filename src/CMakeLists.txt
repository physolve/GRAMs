find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

qt_add_resources(QML_RESOURCES resources.qrc)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_QML_GENERATE_QMLLS_INI ON)

set(EXECUTABLE_NAME "GRAMs")

project(${EXECUTABLE_NAME} VERSION 1.0.0 LANGUAGES CXX)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
endif()

if(NOT DEFINED HPP_CONTROLLERS)
    file(GLOB HPP_CONTROLLERS ${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.h)
endif()

if(NOT DEFINED CPP_CONTROLLERS)
    file(GLOB CPP_CONTROLLERS ${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.cpp)
endif()

if(NOT DEFINED HPP_LIBS)
    file(GLOB HPP_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h)
endif()

if(NOT DEFINED CPP_LIBS)
    file(GLOB CPP_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp)
endif()

if(NOT DEFINED HPP_ADDON)
    file(GLOB HPP_ADDON ${CMAKE_CURRENT_SOURCE_DIR}/addon/*.h)
endif()

if(NOT DEFINED CPP_ADDON)
    file(GLOB CPP_ADDON ${CMAKE_CURRENT_SOURCE_DIR}/addon/*.cpp)
endif()

if(NOT DEFINED HPP_MEASURE)
    file(GLOB HPP_MEASURE ${CMAKE_CURRENT_SOURCE_DIR}/measure/*.h)
endif()

if(NOT DEFINED CPP_MEASURE)
    file(GLOB CPP_MEASURE ${CMAKE_CURRENT_SOURCE_DIR}/measure/*.cpp)
endif()

if(NOT DEFINED HPP_DB)
    file(GLOB HPP_DB ${CMAKE_CURRENT_SOURCE_DIR}/db/*.h)
endif()

if(NOT DEFINED CPP_DB)
    file(GLOB CPP_DB ${CMAKE_CURRENT_SOURCE_DIR}/db/*.cpp)
endif()

if(NOT DEFINED HPP_PLAYPATH)
    file(GLOB HPP_PLAYPATH ${CMAKE_CURRENT_SOURCE_DIR}/playpath/*.h)
endif()

if(NOT DEFINED CPP_PLAYPATH)
    file(GLOB CPP_PLAYPATH ${CMAKE_CURRENT_SOURCE_DIR}/playpath/*.cpp)
endif()

if(NOT DEFINED HPP_ACTIONS)
    file(GLOB HPP_ACTIONS ${CMAKE_CURRENT_SOURCE_DIR}/actions/*.h)
endif()

if(NOT DEFINED CPP_ACTIONS)
    file(GLOB CPP_ACTIONS ${CMAKE_CURRENT_SOURCE_DIR}/actions/*.cpp)
endif()

if(NOT DEFINED HPP_CHARTS)
    file(GLOB HPP_CHARTS ${CMAKE_CURRENT_SOURCE_DIR}/charts/*.h)
endif()

if(NOT DEFINED CPP_CHARTS)
    file(GLOB CPP_CHARTS ${CMAKE_CURRENT_SOURCE_DIR}/charts/*.cpp)
endif()

if (LINUX)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libDAQ)
endif (LINUX)

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SOURCES})
source_group("Controller Header" FILES ${HPP_CONTROLLERS})
source_group("Controller Source" FILES ${CPP_CONTROLLERS})
source_group("Libs Header" FILES ${HPP_LIBS})
source_group("Libs Source" FILES ${CPP_LIBS})

add_definitions(-DQCUSTOMPLOT_USE_OPENGL)

add_subdirectory(runtable)
add_subdirectory(actions/tests)

qt6_add_executable(${EXECUTABLE_NAME} ${SOURCES}
${CPP_CONTROLLERS}
${CPP_LIBS}
${CPP_ADDON}
${CPP_MEASURE}
${CPP_DB}
${CPP_PLAYPATH}
${CPP_ACTIONS}
${CPP_CHARTS}
${QML_RESOURCES})

if (WIN32)
    target_link_libraries(${EXECUTABLE_NAME}
        PRIVATE Qt6::Core
                Qt6::Quick
                Qt6::QuickControls2
                Qt6::Charts
                Qt6::PrintSupport
                Qt6::SerialPort
                Qt6::OpenGL
                Qt6::Sql
                Qt6::Concurrent
                Qt6::TaskTree
                # biodaq
                opengl32
                RuntableLibplugin
        Eigen3::Eigen
    )
else()
    target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE Qt6::Core
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::Charts
            Qt6::PrintSupport
            Qt6::SerialPort
            Qt6::OpenGL
            Qt6::Sql
            Qt6::Concurrent
            Qt6::TaskTree
            biodaq
            RuntableLibplugin
    )
endif()