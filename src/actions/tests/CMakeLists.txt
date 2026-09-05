cmake_minimum_required(VERSION 2.8...4.0)
project(VacuumTreeTests)

enable_testing()

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core TaskTree REQUIRED)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

# .. даёт actions/, ../.. даёт src/ — рецепты включают ../../SensorQuality.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Рецепт «Вакуума» компилируется напрямую — тесты не зависят от
# ValveControl/biodaq (все швы через VacuumTreeContext).
add_executable(VacuumTreeTests
    test_vacuum_tree.cpp
    test_supply_tree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../recipes/RecipeCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../recipes/VacuumTaskTree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../recipes/SupplyTaskTree.cpp
)

target_link_libraries(VacuumTreeTests
    PRIVATE
    Qt6::Core
    Qt6::TaskTree
    gtest
)

include(GoogleTest)
gtest_discover_tests(VacuumTreeTests)
