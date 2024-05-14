cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0099 NEW)

set(EXECUTABLE_NAME "GRAMs")

project(${EXECUTABLE_NAME} VERSION 0.1 LANGUAGES CXX)

#set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 6.2.4 COMPONENTS Core Quick QuickControls2 Charts PrintSupport OpenGL REQUIRED) # Charts QuickControls2   LinguistTools
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})
set(MY_UNITTESTS TRUE)
include(config.cmake)

#add_subdirectory(modules/FirstPlugin)
#add_subdirectory(modules/SecondPlugin)
#add_subdirectory(modules/Controls)

add_subdirectory(modules/Style)

# if(${MY_UNITTESTS})
#     add_subdirectory(tests/UnitTestQML)
#     add_subdirectory(tests/UnitTestCpp)
# endif()
#TODO: add resources file
qt_add_resources(IMAGE_RESOURCES resources.qrc)

file(GLOB CPP_SOURCES src/*.cpp)
file(GLOB HPP_SOURCES src/*.h)

file(GLOB CPP_CONTROLLERS src/controllers/*.cpp)
file(GLOB HPP_CONTROLLERS src/controllers/*.h)

file(GLOB CPP_LIBS lib/*.cpp)
file(GLOB HPP_LIBS lib/*.h)

qt_add_executable(${EXECUTABLE_NAME}
    ${CPP_SOURCES} ${HPP_SOURCES}
    ${CPP_CONTROLLERS} ${HPP_CONTROLLERS}
    ${CPP_LIBS} ${HPP_LIBS}
    ${IMAGE_RESOURCES}
)

#add_definitions(-DQCUSTOMPLOT_USE_OPENGL)


#file(GLOB QML_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} qml/*.qml ) #qml/components/*.qml <- for custom modules
#source_group("Qml Files" FILES ${QML_SOURCES})

#qt_add_qml_module(${EXECUTABLE_NAME}
#    URI ${EXECUTABLE_NAME}
#    VERSION 1.0
#    QML_FILES ${QML_SOURCES}
#)

#set_target_properties(${EXECUTABLE_NAME} PROPERTIES
#    MACOSX_BUNDLE FALSE
#    WIN32_EXECUTABLE TRUE
#)

#target_include_directories(${EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE Qt6::Core
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::Charts
            Qt6::PrintSupport
            Qt6::OpenGL
    #PRIVATE Qt6::QuickControls2
    #PRIVA5TE Qt6::Charts
    #PRIVATE Qt6::PrintSupport
    #PRIVATE Controlsplugin
    #PRIVATE FirstPluginplugin
    #PRIVATE SecondPluginplugin
    Eigen3::Eigen
    PRIVATE Styleplugin
)

#file(GLOB_RECURSE QML_SOURCES2 RELATIVE ${CMAKE_SOURCE_DIR} *.qml)

# TODO: using lupdate create translate
# qt_add_translations(${EXECUTABLE_NAME} TS_FILES
#         ${EXECUTABLE_NAME}_ru_RU.ts
#         ${EXECUTABLE_NAME}_en_EN.ts
#     SOURCES ${QML_SOURCES2} ${CPP_SOURCES} ${HPP_SOURCES}
#     RESOURCE_PREFIX "/translations"
# )

#install(TARGETS ${EXECUTABLE_NAME}
#    BUNDLE DESTINATION .
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)
