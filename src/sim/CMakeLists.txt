# SimCore — подсистема демо-данных датчиков (docs/sim/README.md).
# Чистые части без GUI и без biodaq: профиль, интерполяция, машина фаз,
# JSON-RPC, подменные источник данных и порт клапанов. GRAMs линкует её,
# тесты SimTests — тоже.

find_package(Qt6 6.11 REQUIRED COMPONENTS Core StateMachine Network HttpServer)

set(CMAKE_AUTOMOC ON)

qt_add_library(SimCore STATIC
    core/SimTypes.h
    core/TrackEngine.h
    core/TrackEngine.cpp
    core/SimProfile.h
    core/SimProfile.cpp
    core/SimCatalog.h
    core/SimCatalog.cpp
    core/SimClock.h
    core/SimEvents.h
    core/SimErrors.h
    core/SimStates.h
    core/SimStates.cpp
    core/SimController.h
    core/SimController.cpp
    rpc/SimRpcDispatcher.h
    rpc/SimRpcDispatcher.cpp
    rpc/SimRpcServer.h
    rpc/SimRpcServer.cpp
    hw/SensorInverse.h
    hw/SensorInverse.cpp
    hw/SimSensorSource.h
    hw/SimSensorSource.cpp
    hw/SimValveEcho.h
    hw/SimValveEcho.cpp
)

# src/ — ради интерфейсов ISensorSource / IDoPort и SensorQuality.h.
target_include_directories(SimCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(SimCore
    PUBLIC
    Qt6::Core
    Qt6::StateMachine
    Qt6::Network
    Qt6::HttpServer
)

# Новый код — без предупреждений (A.3); Qt-заголовки приходят как внешние.
if (MSVC)
    target_compile_options(SimCore PRIVATE /W4 /WX /utf-8 /external:W0)
else()
    target_compile_options(SimCore PRIVATE -Wall -Wextra -Werror)
endif()

add_subdirectory(tests)
