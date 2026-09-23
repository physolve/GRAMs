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

# Существующий код GRAMs, нужный тестам «туда-обратно», — отдельной
# библиотекой: на него не распространяется /WX нового кода.
add_library(SimTestsGramsData STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../DataCollection.cpp
)
target_include_directories(SimTestsGramsData PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(SimTestsGramsData PUBLIC Qt6::Core)

add_executable(SimTests
    test_main.cpp
    test_track_engine.cpp
    test_sim_profile.cpp
    test_sim_state_machine.cpp
    test_sim_rpc.cpp
    test_sim_hw.cpp
    test_sim_options.cpp
)

find_package(Qt6 REQUIRED COMPONENTS Test Concurrent)
target_link_libraries(SimTests PRIVATE SimCore SimTestsGramsData Qt6::Test Qt6::Concurrent gtest)

# Эталонные профили и фикстуры лежат в tests/sim/profiles/ корня репозитория.
target_compile_definitions(SimTests PRIVATE
    SIM_PROFILES_DIR="${PROJECT_SOURCE_DIR}/../../../tests/sim/profiles"
    GRAMS_PROFILE_JSON="${PROJECT_SOURCE_DIR}/../../../profile/GRAMsPfp.json")

if (MSVC)
    target_compile_options(SimTests PRIVATE /W4 /WX /utf-8 /external:W0)
endif()

include(GoogleTest)
gtest_discover_tests(SimTests)

# ── Интеграционный тест «Вакуума» на демо-данных ─────────────────────────────
# Настоящие DataAcquisition, ValveControl, Security и рецепт «Вакуума»; biodaq
# не вызывается (источник данных и порт клапанов — демо). Существующий код
# собирается без /WX: предупреждения в нём — не предмет этой ветки.
find_package(Qt6 REQUIRED COMPONENTS SerialPort Sql Widgets PrintSupport OpenGL TaskTree Quick)
set(GRAMS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_library(SimGramsRuntime STATIC
    ${GRAMS_SRC}/DataAcquisition.cpp
    ${GRAMS_SRC}/FilterView.cpp
    ${GRAMS_SRC}/Security.cpp
    ${GRAMS_SRC}/ValveControl.cpp
    ${GRAMS_SRC}/controllers/AdvantechCtrl.cpp
    ${GRAMS_SRC}/controllers/ControllerInfo.cpp
    ${GRAMS_SRC}/controllers/SerialCtrl.cpp
    ${GRAMS_SRC}/controllers/VoltageFilter.cpp
    ${GRAMS_SRC}/lib/kalman.cpp
    ${GRAMS_SRC}/actions/recipes/RecipeCommon.cpp
    ${GRAMS_SRC}/actions/recipes/VacuumTaskTree.cpp
    # Воркеры с проверкой давления Security: «Тест клапанов» и RegimeWorkerBase.
    ${GRAMS_SRC}/actions/RegimeLogger.cpp
    ${GRAMS_SRC}/actions/workers/RegimeWorkers.cpp
    ${GRAMS_SRC}/actions/workers/ValveTestWorker.cpp
    ${GRAMS_SRC}/actions/workers/VacuumRunMonitor.cpp
    # Оркестратор режимов: закрытие клапанов по концу режима и передача
    # событий Security воркеру проверяются через настоящий RegimeTaskTree.
    ${GRAMS_SRC}/actions/RegimeTaskTree.cpp
    ${GRAMS_SRC}/actions/workers/ChainWorkers.cpp
    ${GRAMS_SRC}/actions/recipes/SupplyTaskTree.cpp
    ${GRAMS_SRC}/actions/recipes/LeakageTaskTree.cpp
    integration_stubs.cpp
)
target_include_directories(SimGramsRuntime PUBLIC ${GRAMS_SRC} ${GRAMS_SRC}/actions)
if (TARGET Eigen3::Eigen)
    target_link_libraries(SimGramsRuntime PUBLIC Eigen3::Eigen)
else()
    target_include_directories(SimGramsRuntime PUBLIC ${EIGEN3_INCLUDE_DIR})
endif()
target_compile_definitions(SimGramsRuntime PUBLIC QCUSTOMPLOT_USE_OPENGL)
target_link_libraries(SimGramsRuntime PUBLIC
    SimTestsGramsData RuntableLib Qt6::Core Qt6::Qml Qt6::SerialPort Qt6::Sql Qt6::Widgets Qt6::PrintSupport Qt6::OpenGL Qt6::TaskTree Qt6::Quick)
if (MSVC)
    target_compile_options(SimGramsRuntime PRIVATE /utf-8)
endif()

add_executable(SimIntegrationTests
    test_main.cpp
    test_sim_vacuum_integration.cpp
    test_sim_valves_integration.cpp
    test_sim_security_range.cpp
)
target_link_libraries(SimIntegrationTests PRIVATE SimCore SimGramsRuntime gtest)
target_compile_definitions(SimIntegrationTests PRIVATE
    SIM_PROFILES_DIR="${PROJECT_SOURCE_DIR}/../../../tests/sim/profiles"
    GRAMS_PROFILE_JSON="${PROJECT_SOURCE_DIR}/../../../profile/GRAMsPfp.json"
    GRAMS_ADDONS_JSON="${PROJECT_SOURCE_DIR}/../../../profile/addons.json")
if (MSVC)
    target_compile_options(SimIntegrationTests PRIVATE /utf-8)
endif()
# Рабочая папка — корень репозитория, как у GRAMs.exe: FilterView читает
# profile/kalman.json и без неё падает на отладочном assert Qt.
gtest_discover_tests(SimIntegrationTests
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../../..
    DISCOVERY_TIMEOUT 60
    PROPERTIES TIMEOUT 300)
