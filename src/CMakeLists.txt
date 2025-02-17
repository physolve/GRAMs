find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})
# find_package(libbio REQUIRED)
qt_add_resources(QML_RESOURCES resources.qrc)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(EXECUTABLE_NAME "GRAMs")

project(${EXECUTABLE_NAME} VERSION 1.0.0 LANGUAGES CXX)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fstrict-aliasing -pedantic-errors -pedantic -Wno-deprecated-declarations -Wno-unused-variable")
SET( CMAKE_CXX_FLAGS  "-lbiodaq")

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

if (LINUX)            
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libDAQ)
endif (LINUX)

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SOURCES})
source_group("Controller Header" FILES ${HPP_CONTROLLERS})
source_group("Controller Source" FILES ${CPP_CONTROLLERS})
source_group("Libs Header" FILES ${HPP_LIBS})
source_group("Libs Source" FILES ${CPP_LIBS})

qt6_add_executable(${EXECUTABLE_NAME} ${HEADERS} ${SOURCES} 
${CPP_CONTROLLERS} ${HPP_CONTROLLERS}
${CPP_LIBS} ${HPP_LIBS} ${QML_RESOURCES})

target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE Qt6::Core
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::Charts
            Qt6::PrintSupport
            Qt6::OpenGL
            Eigen3::Eigen
)