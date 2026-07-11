# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**GRAMs** (Gas Reaction Automated Machine) — Qt Quick / C++ приложение для автоматического управления установкой GRAM50, реализующей **волюметрический метод Сивертса** (исследование сорбции/десорбции водорода в гидридных материалах). Количество поглощённого H₂ определяется по изменению давления в калиброванных объёмах до и после контакта газа с образцом.

- **Type:** Qt Quick Application + embedded static QML library (`RuntableLib`)
- **Platform:** Linux (ветка `linux-dev`, основная цель) / Windows (сборка без biodaq)
- **Previous version:** GramQt (отдельный репозиторий) — источник алгоритмов для переноса

## Build Commands

> **Требование:** `cl.exe` требует MSVC-окружения. Есть два способа.

### Способ А — интерактивный (обычный терминал)

Открой **x64 Native Tools Command Prompt for VS 2022** и выполняй команды там.

### Способ Б — агентный запуск из обычного PowerShell / `claude -p`

Всегда инициализируй окружение перед cmake через `cmd /c`:

```powershell
# ── Переменная окружения MSVC ──────────────────────────────────────────────────
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# ── Первичная конфигурация (чистая папка build/) ──────────────────────────────
cmd /c "`"$vcvars`" && cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/msvc2022_64 -DEigen3_DIR=F:/testFilterLib/eigen/build"

# ── Debug сборка ───────────────────────────────────────────────────────────────
cmd /c "`"$vcvars`" && cmake --build build --parallel"

# ── Release сборка ─────────────────────────────────────────────────────────────
cmd /c "`"$vcvars`" && cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/msvc2022_64 -DEigen3_DIR=F:/testFilterLib/eigen/build"
cmd /c "`"$vcvars`" && cmake --build build --parallel"

# ── Запуск приложения ──────────────────────────────────────────────────────────
.\build\src\GRAMs.exe

# ── Запуск всех тестов (обе gtest-сюиты через ctest) ──────────────────────────
Push-Location build; ctest --output-on-failure; Pop-Location

# ── Сборка/запуск конкретной сюиты ────────────────────────────────────────────
cmd /c "`"$vcvars`" && cmake --build build --target ProtoTableTests --parallel"
cmd /c "`"$vcvars`" && cmake --build build --target VacuumTreeTests --parallel"

# ── Запуск одного теста по маске ──────────────────────────────────────────────
.\build\src\runtable\tests\ProtoTableTests.exe --gtest_filter=*RegimeManager*
.\build\src\actions\tests\VacuumTreeTests.exe  --gtest_filter=*Pumpdown*
```

**Тестовые сюиты (GoogleTest, оба через `gtest_discover_tests` → `ctest`):**

| Сюита | Расположение | Покрытие | Особенность сборки |
|---|---|---|---|
| `ProtoTableTests` | `src/runtable/tests/` | `RegimeManager` (state machine, External Module API), `ProtoTableModel`, расчёты времени | линкует `RuntableLib` + `Qt6::Test/Core/Qml` |
| `VacuumTreeTests` | `src/actions/tests/` | рецепт «Вакуума» на TaskTree | компилирует `VacuumTaskTree.cpp` напрямую; все швы к железу — через `VacuumTreeContext`, поэтому **не зависит от `ValveControl`/biodaq** (`Qt6::Core` + `Qt6::TaskTree` + `gtest`) |

> Паттерн `cmd /c "\"vcvars64.bat\" && <команда>"` передаёт инициализированное MSVC-окружение в дочерний процесс cmd, а затем выполняет cmake. Переменные среды не просачиваются обратно в PowerShell — это нормально, каждый вызов самодостаточен.

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
| `Qt::Test` | GoogleTest-интеграция (`ProtoTableTests` в `src/runtable/tests/`) |
| `Qt::TaskTree` | Оркестрация режимов (`RegimeTaskTree`, рецепт «Вакуума`); также `VacuumTreeTests` |

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

- Конфигурация железа читается из JSON-профилей (`profile/`) через `Initialize` и передаётся в подсистемы через typed `Q_GADGET`-структуры. Файлы профиля: `GRAMsPfp.json` (главный), `chambers.json`, `addons.json`, `kalman.json`, `regime_a.json`
- GUI-структуры намеренно живут в `Quartile` — это архитектурное решение, не менять
- `split` / `collapse` в `NodePressure` — физически верифицированная логика, не рефакторить без понимания физики
- `ControlDO` передаётся в `ActionHandler` явно — паттерн не менять

## Active Work

> Этот раздел обновляется вручную по мере продвижения.

**Текущий приоритет (в работе):** перенос автоматических режимов из GramQt в GRAMs. Готовы «в коде»: **Вакуум** (Ф1–Ф3) и **Тест клапанов**; остальные — заглушки или ожидают.

Очерёдность режимов:

Статусы (4 уровня, каждый обязан иметь доказательство `файл:метод:строка`):

| Статус | Значение |
|---|---|
| **Заглушка** | Только `qDebug` + `// TODO` в 4 методах |
| **Структура (TODO)** | Каркас есть, но клапан `= ""` / completion закомментирован / сенсор не подключён |
| **Реализован (код)** | Все 4 метода рабочие; не проверено на железе |
| **Проверено на железе** | Реальный прогон на Linux + biodaq |

| Режим | Статус | Доказательство / что осталось |
|---|---|---|
| **Вакуум** | **Реализован (код)** Ф1–Ф3 (s1–s24); **Заглушка** Ф4–Ф8 — `docs/regimes/vacuum.md` | `VacuumTaskTree.cpp:buildVacuumRecipe:339` рецепт на TaskTree; `buildPumpdown:304` Ф4+ заглушка; 17 тестов `VacuumTreeTests` проходят; не проверено на железе |
| **Режим в** | **Заглушка** — `docs/regimes/regime-b.md` | `RegimeWorkers.cpp:417–441` все 4 метода = `qDebug` + `// TODO` |
| **Режим г** | **Заглушка** — `docs/regimes/regime-g.md` | `RegimeWorkers.cpp:445–465` аналогично Режиму в |
| **Тест клапанов** | **Реализован (код)** — `docs/regimes/valve-test.md` | `ValveTestWorker.cpp` полная state machine; не верифицирован на железе (Windows) |
| **Напуск** | Ожидает | Воркер не создан |
| **Натекание** | Ожидает | Воркер не создан |
| **Калибровка** | Ожидает | Воркер не создан; узкое место для SYSTEST |
| **SOAK** | Ожидает | Воркер не создан |
| **PCI** | Ожидает | Воркер не создан |
| **SYSTEST** | Ожидает | Воркер не создан |

**Алгоритм добавления нового режима:**
1. Изучить реализацию в GramQt
2. Создать воркер: наследовать `RegimeWorkerBase` (или `ValveTestWorker` для клапанных сценариев) и переопределить `onExecutionPhaseStart`, `onExecutionTick`, `isExecutionComplete`, `onExecutionPhaseEnd`. Для сложных последовательностей с ветвлениями/cleanup-гарантиями — паттерн `VacuumRegimeWorker`: standalone-воркер + чистый TaskTree-рецепт с швами `std::function` (см. `src/actions/VacuumTaskTree.h`), тестируемый без biodaq
3. Зарегистрировать имя режима → воркер в `RegimeTaskTree::buildRegimeGroup()`
4. Добавить режим в `RunTable.qml` меню «Добавить»
5. Отладка → верификация на LaNi₅ (плато сорбции ~2 бар при 25°C)

## Important Notes

- **Qt TaskTree** (Qt 6.11) — используется в namespace `QtTaskTree::` (заголовок `<qtasktree.h>`). В Qt 6.11 это **Technology Preview**, API может меняться между минорными версиями.
- `Qt::Concurrent` уже подключён в обоих блоках `target_link_libraries` (`WIN32` / `else`) в `src/CMakeLists.txt` — дополнительных правок CMake для TaskTree не требуется.
- Главный CMake-таргет: **`GRAMs`** — именно к нему добавлять новые `target_link_libraries`.
- Лог пишется в `data/GRAMs-log.txt` относительно рабочей директории запуска; папка `data/` создаётся автоматически. Данные режимов: `data/leakageData/` (натекание), `data/supplyData/` (напуск), `data/regime_log.db` (SQLite лог регимов).
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

### Async actions: `RegimeTaskTree` (основной путь)

`src/actions/RegimeTaskTree.h` — оркестратор автоматических режимов на базе Qt TaskTree (`Tasking::` namespace). Зарегистрирован как QML singleton `RegimeTaskTree`. Управляет последовательным запуском всех `Waiting`-режимов из `RegimeManager`.

**Worker-паттерн** (`src/actions/RegimeWorkers.h`):
- `RegimeWorkerBase` — базовый класс; работает в главном event loop через `QTimer` (1-секундные тики); поддерживает `pause`/`resume` через сигналы `RegimeTaskTree::pauseRequested/resumeRequested`
- Двухфазный цикл на повтор: **Condition phase** (ожидание условия: время или температура) → **Execution phase** (логика режима)
- Чтобы добавить режим — переопределить 4 виртуальных метода: `onExecutionPhaseStart`, `onExecutionTick`, `isExecutionComplete`, `onExecutionPhaseEnd`
- Конкретные воркеры: `VacuumRegimeWorker`, `RegimeBWorker`, `RegimeGWorker`
- `ValveTestWorker` (`src/actions/ValveTestWorker.h`) — отдельный воркер с конфигурируемой последовательностью шагов клапанов; конфигурируется из QML через `RegimeTaskTree.setValveTestSteps()`

**Логирование** (`src/actions/RegimeLogger.h`): отдельная SQLite-база `data/regime_log.db` (схема: `regime_runs` + `regime_events`). Не путать с `GramStateDB` (состояния физических объёмов).

**Устаревший путь** (`src/actions/ActionHandler.h`): использует `QtConcurrent::run` + `QFutureWatcher<int>`. Содержит `InletAction` (одно событие напуска) и закомментированный `DataControlAction`. Оставлен для совместимости, но новые режимы пишутся через `RegimeTaskTree`.

### Experiment scheduler: `RegimeManager` + `RuntableLib`

`src/runtable/` — отдельный статический QML-модуль (`com.grams.prototable`; QML import: `import com.grams.prototable 1.0`). Состояния режима (`RegimeEnums::State`): `Waiting → Running → Paused → Done / Stopped / Skipped / Error` (двухфазный: condition + execution внутри Running). `RegimeManager` предоставляет External Module API (методы `startRegimeExecution`, `updateConditionProgress`, `confirmConditionCompletion`, `updateRegimeProgress`, `completeCurrentRepeat` и т.д.) — воркеры обязаны вызывать именно эти методы для отражения прогресса в UI.

### Pressure simulation: `PlayPressure`

`src/playpath/PlayPressure.h` — симулятор перераспределения давления по газовому тракту без реального оборудования. Получает указатели на `AddRemoveQuartile`, `StorageQuartile`, `ReactionQuartile` и вычисляет виртуальные давления методами `play()` / `playWithAccuum()`. Результат доступен из QML через `app.guiPresChange` / `app.guiPresTotal`. Использует `guiPressureTarget` Q_GADGET для целевых параметров, задаваемых из QML.

### Reaction chamber: `Chamber`

`src/measure/Chamber.h` — модель реакционной камеры. Хранит `VolumeObject` и параметры `ChamberParameters` (объём, максимальное давление, статус открытия). Используется `ReactionQuartile` для описания физической геометрии реактора.

### Persistence

- `src/db/GramStateDB.h` — SQLite: состояния объёмов банок, начало/конец Action, параметры экспериментов; быстрые данные → CSV в папку эксперимента
- `src/TimeStamp.h` — снимок последнего известного физического состояния; используется как начальное значение виртуальных объёмов после перезапуска

### QML UI (`src/qml/`)

Точка входа: `src/qml/Main.qml`. Навигация через `SideMenu.qml`. `Grams` зарегистрирован как context property `app`; QML получает данные через `app.guiPres`, `app.guiTemp`, `app.guiPresVirtual`, `app.guiValve`.

Мнемосхемы: `src/qml/mnemo/GRAM50_mnemo/` и `src/qml/mnemo/GRAM300_mnemo/` — для двух вариантов установки. Точка входа каждой: `*Content/App.qml`.

`ValveTestSetup.qml` (`src/qml/`) — страница конфигурации «Теста клапанов»: собирает последовательность шагов и передаёт её в `RegimeTaskTree.setValveTestSteps()` (док: `src/qml/doc/ValveTestSetup.md`).

> **Важно:** QML экспериментального планировщика (в т.ч. `RunTable.qml` с меню «Добавить») лежит **не** в `src/qml/`, а в модуле `RuntableLib` — `src/runtable/RunTable.qml` (плюс делегаты `RegimeDelegate.qml`, `StateDelegate.qml`, `ConditionCell.qml`, `TimeProgressBar.qml`). Импорт: `import com.grams.prototable 1.0`.
