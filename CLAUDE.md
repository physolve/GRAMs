# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**GRAMs** (Gas Reaction Automated Machine) — Qt Quick / C++ приложение для автоматического управления установкой GRAM50, реализующей **волюметрический метод Сивертса** (исследование сорбции/десорбции водорода в гидридных материалах). Количество поглощённого H₂ определяется по изменению давления в калиброванных объёмах до и после контакта газа с образцом.

- **Type:** Qt Quick Application + embedded static QML library (`RuntableLib`)
- **Platform:** Linux (ветка `linux-dev`, основная цель) / Windows (сборка без biodaq)
- **Previous version:** GramQt (отдельный репозиторий) — источник алгоритмов для переноса

## Build Commands

> **Требование:** все команды выполнять в **x64 Native Tools Command Prompt for VS 2022** (или Developer PowerShell), иначе Ninja не найдёт `cl.exe`.

```powershell
# ── Первичная конфигурация (чистая папка build/) ──────────────────────────────
cmake -B build -S . -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64" `
  -DEigen3_DIR="F:/testFilterLib/eigen/build"

# ── Debug сборка ───────────────────────────────────────────────────────────────
cmake --build build --parallel

# ── Release сборка (пересконфигурировать с новым типом) ───────────────────────
cmake -B build -S . -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64" `
  -DEigen3_DIR="F:/testFilterLib/eigen/build"
cmake --build build --parallel

# ── Запуск приложения ──────────────────────────────────────────────────────────
.\build\src\GRAMs.exe

# ── Запуск всех тестов ─────────────────────────────────────────────────────────
Push-Location build; ctest --output-on-failure; Pop-Location

# ── Запуск одного теста по маске ──────────────────────────────────────────────
.\build\src\runtable\tests\ProtoTableTests.exe --gtest_filter=*RegimeManager*
```

**Зависимости, найденные в текущей сборке:**

| Переменная | Значение |
|---|---|
| `Qt6_DIR` | `C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6` |
| `Eigen3_DIR` | `F:/testFilterLib/eigen/build` |
| `CMAKE_CXX_COMPILER` | `C:/Program Files/Microsoft Visual Studio/2022/Community/…/cl.exe` |
| Generator | `Ninja` (одноконфигурационный — `Debug` или `Release` задаётся при конфигурации) |

> Профиль установки жёстко прописан в `src/main.cpp`: `const auto &curInitProfile = QString("GRAM50");`
> GoogleTest подтягивается автоматически через `FetchContent` при первом конфигурировании.

## Qt Configuration

- **Qt version:** 6.11.1
- **Install path:** `C:/Qt/6.11.1/msvc2022_64`
- **Compiler:** MSVC 2022 (`cl.exe`, toolset 14.37)
- **Architecture:** x64
- **Generator:** Ninja (одноконфигурационный)

## Key Modules

| Модуль | Назначение в проекте |
|---|---|
| `Qt::Core` | Базовый — везде |
| `Qt::Quick` | QML runtime |
| `Qt::QuickControls2` | QML Controls |
| `Qt::Concurrent` | `QtConcurrent::run`, `QFuture`, `QPromise` (ActionHandler) |
| `Qt::Charts` | Графики |
| `Qt::SerialPort` | Вакуумметр по serial |
| `Qt::OpenGL` | QCustomPlot с OpenGL (`QCUSTOMPLOT_USE_OPENGL`) |
| `Qt::Sql` | SQLite — GramStateDB |
| `Qt::PrintSupport` | Печать |
| `Qt::Qml` | RuntableLib QML-модуль |
| `Qt::Test` | GoogleTest-интеграция (тесты runtable) |

**Сторонние библиотеки:**
- **Eigen3** — матричная алгебра (Kalman filter), системная зависимость
- **QCustomPlot** — `src/lib/qcustomplot.{h,cpp}`, OpenGL-режим
- **GoogleTest** — `FetchContent` (GitHub ZIP), только тесты
- **biodaq / libDAQ** — prebuilt `.so` в `src/libDAQ/`, только Linux (DAQ-железо Advantech)

## Code Conventions

```
m_      — члены класса              (m_pressure, m_valveState)
v_      — виртуальные объекты       (v_pressureRange, v_safeModule)
ptr[]   — массивы указателей        — всегда проверять на nullptr перед использованием
```

- Конфигурация железа читается из JSON-профиля (`profile/`) через `Initialize` и передаётся в подсистемы через typed `Q_GADGET`-структуры
- GUI-структуры намеренно живут в `Quartile` — это архитектурное решение, не менять
- `split` / `collapse` в `NodePressure` — физически верифицированная логика, не рефакторить без понимания физики
- `ControlDO` передаётся в `ActionHandler` явно — паттерн не менять

## Active Work

> Этот раздел обновляется вручную по мере продвижения.

**Текущий приоритет (лето 2025):** перенос автоматических режимов из GramQt в GRAMs.

Очерёдность режимов:

| Режим | Статус |
|---|---|
| **Вакуум** | В работе (первый в очереди) |
| **Напуск** | Ожидает |
| **Натекание** | Ожидает |
| **Калибровка** | Ожидает |
| **SOAK** | Ожидает |
| **PCI** | Ожидает |
| **SYSTEST** | Ожидает |

**Алгоритм переноса каждого режима:**
1. Изучить реализацию в GramQt
2. Адаптировать под архитектуру GRAMs (Quartile-граф, ActionHandler, Strategy)
3. Реализовать соответствующую Strategy-стратегию
4. Отладка → верификация на LaNi₅ (плато сорбции ~2 бар при 25°C)

## Important Notes

- **Qt TaskTree** (Qt 6.11) — используется в namespace `Tasking::`. В Qt 6.11 это **Technology Preview**, API может меняться между минорными версиями.
- `Qt::Concurrent` уже подключён в обоих блоках `target_link_libraries` (`WIN32` / `else`) в `src/CMakeLists.txt` — дополнительных правок CMake для TaskTree не требуется.
- Главный CMake-таргет: **`GRAMs`** — именно к нему добавлять новые `target_link_libraries`.
- Лог пишется в `data/GRAMs-log.txt` относительно рабочей директории запуска; папка `data/` создаётся автоматически.
- На Windows `biodaq` исключён, добавляется `opengl32`; на Linux линкуется `biodaq` из `src/libDAQ/`.

## Architecture

### Application root: `Grams`

`src/Grams.h` / `src/Grams.cpp` — центральный класс, наследует `QApplication`. Владеет всеми domain-объектами и связывает их в серии `init*()` методов (`initDigitalData`, `initAnalogData`, `initCharts`, `initSafeModule`, `initActionHandler`, …). Главный polling-цикл: `softTimer` → `softEvent()`.

### Hardware profile: `Initialize`

`src/Initialize.h` — читает JSON-профиль (напр. `GRAM50`) из `profile/` и заполняет типизированные структуры (`daqParameters`, `hardwareParameters`, `storageQuarParameters`, `reactionQuarParameters`, `securityParameters`, `vacuumParameters`). Структуры — `Q_GADGET`, доступны из QML напрямую.

### Data acquisition: `DataAcquisition`

`src/DataAcquisition.h` — управляет Advantech DAQ (`AdvantechBuff` — AI давление, `AdvantechAI` — AI температура) и serial вакуумметром (`VacuumController`). Данные пишутся через сырые указатели на объекты `ControllerData` / `FilterData` / `DataCollection`, живущие в `Grams`. Kalman-фильтрация — в `FilterView`.

### Data types (`src/DataCollection.h`)

```
DataCollection      — именованный кольцевой буфер, коэффициент altUnit
  ControllerData    — линейная калибровка (A·x + B) сырого напряжения датчика
  FilterData        — накопительный буфер для входа Kalman-фильтра
  QuartileData      — снимок давления в газовом объёме
  MolesData         — накопитель количества молей на объём
  NodeData          — узел давления (расчёт равновесия)
```

### Valve control: `ValveControl`

`src/ValveControl.h` — запись состояний DO в `AdvantechDO`. Все записи проходят через `Security` (проверка противоречий, `checkOpenChamber`). Список физических клапанов (`vAR1`…`vR5`) живёт в `Grams.h`.

### Pressure domain: Quartile subsystem (`src/addon/`)

Газовый тракт разбит на четыре зоны, каждая — подкласс `Quartile` с картами `VolumeObject`, узлами `NodePressure` и указателями на `ControllerData` / `Valve`:

| Класс | Зона |
|---|---|
| `AddRemoveQuartile` | Сторона напуска/стравливания газа |
| `StorageQuartile` | Баллоны высокого давления (C1, C2, C3, B) |
| `ReactionQuartile` | Область реакционной камеры |
| `SecondLineQuartile` | Вторая газовая линия / масс-спектрометр |

`NodePressure` — модель узла графа: хранит давление в объёме, реализует `split` (разделение при открытии клапана) и `collapse` (объединение при закрытии). Центральная абстракция — не менять без понимания физики.

`QuartileManager` — агрегирует все четыре quartile, единая точка входа для запросов перераспределения давления.

`StrategyBuilder` (`src/addon/StrategyBuilder.h`) — строит объекты `SupplyPort` / `GasLeakage` из состояния quartile. Strategy-классы содержат логику режима, отделены от GUI.

### Async actions: `ActionHandler`

`src/actions/ActionHandler.h` — точка входа для всех автоматических режимов. Использует `QtConcurrent::run` + `QFutureWatcher<int>`. Режим = последовательность Action-шагов, каждый со своей стратегией.

- `InletAction` — одно событие напуска (открыть клапан на время); worker через `QPromise<int>` + `QThread::msleep`
- `DataControlAction` — управление DAQ во время action (сейчас закомментирован)
- Получает указатели на `ValveControl`, `DataAcquisition`, `Security` через injection в `Grams`

### Experiment scheduler: `RegimeManager` + `RuntableLib`

`src/runtable/` — отдельный QML-модуль (`com.grams.prototable`). Состояния режима: `Idle → Running (condition) → Running (execution) → Done / Skipped / Error`.

### Persistence

- `src/db/GramStateDB.h` — SQLite: состояния объёмов банок, начало/конец Action, параметры экспериментов; быстрые данные → CSV в папку эксперимента
- `src/TimeStamp.h` — снимок последнего известного физического состояния; используется как начальное значение виртуальных объёмов после перезапуска

### QML UI (`src/qml/`)

Точка входа: `src/qml/Main.qml`. Навигация через `SideMenu.qml`. `Grams` зарегистрирован как context property `app`; QML получает данные через `app.guiPres`, `app.guiTemp`, `app.guiPresVirtual`, `app.guiValve`.
