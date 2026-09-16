cmake_minimum_required(VERSION 3.21)
project(SimTests)

enable_testing()

set(CMAKE_AUTOMOC ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(SimTests
    test_main.cpp
    test_track_engine.cpp
    test_sim_profile.cpp
    test_sim_state_machine.cpp
)

target_link_libraries(SimTests PRIVATE SimCore gtest)

# Эталонные профили и фикстуры лежат в tests/sim/profiles/ корня репозитория.
target_compile_definitions(SimTests PRIVATE
    SIM_PROFILES_DIR="${PROJECT_SOURCE_DIR}/../../../tests/sim/profiles"
    GRAMS_PROFILE_JSON="${PROJECT_SOURCE_DIR}/../../../profile/GRAMsPfp.json")

if (MSVC)
    target_compile_options(SimTests PRIVATE /W4 /WX /utf-8 /external:W0)
endif()

include(GoogleTest)
gtest_discover_tests(SimTests)
