find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

qt_add_resources(QML_RESOURCES resources.qrc)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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

qt6_add_executable(${EXECUTABLE_NAME} ${HEADERS} ${SOURCES} 
${CPP_CONTROLLERS} ${HPP_CONTROLLERS}
${CPP_LIBS} ${HPP_LIBS} 
${CPP_ADDON} ${HPP_ADDON}
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
                # biodaq
                opengl32
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
            biodaq
    )
endif()