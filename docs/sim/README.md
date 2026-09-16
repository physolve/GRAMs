# Демо-данные датчиков (демо-режим GRAMs)

Демо-режим подменяет показания датчиков и порт клапанов, поэтому режимы (в первую
очередь «Вакуум») работают без стенда. Показания задаёт **профиль**: фазы,
ключевые точки и интерполяция. Профиль исполняет машина `QStateMachine`, а
управляет ею gram-db-viewer по JSON-RPC. Режимы, `Security` и квартили о демо не
знают: подмена сделана ниже них.

Контракт RPC и формат профиля описаны в [`rpc-contract.md`](rpc-contract.md).

## Запуск

```powershell
# без подключённого железа: плат Advantech из профиля и портов ДВ301/ДВ302
.\build\src\GRAMs.exe --sim                       # RPC на 127.0.0.1:8770
.\build\src\GRAMs.exe --sim --sim-rpc-port 8779 --sim-token s3cret
$env:GRAMS_SIM = "1"; .\build\src\GRAMs.exe       # то же через окружение

.\tools\sim_rpc_smoke.ps1 -Port 8770              # дымовая проверка RPC
```

| Флаг | Переменная окружения | По умолчанию |
|---|---|---|
| `--sim` | `GRAMS_SIM=1` | выключено |
| `--sim-rpc-port <n>` | `GRAMS_SIM_RPC_PORT` | 8770 |
| `--sim-token <t>` | `GRAMS_SIM_TOKEN` | нет (заголовок не проверяется) |

У флагов командной строки приоритет над окружением. Если порт указан неверно,
демо-режим не включается, а причина пишется в лог.

Запускать нужно из корня репозитория, как и обычно: GRAMs читает папку `profile/`
из рабочей директории.

## Когда демо разрешено

| Условие | Что происходит |
|---|---|
| Нет `--sim` | Ничего из `src/sim/` не создаётся, порт не открывается |
| `--sim` и нет железа | RPC поднят, источник данных и клапаны подменены, на экране плашка |
| `--sim` и есть железо | RPC поднят и на управляющие методы отвечает `SIM_NOT_ALLOWED` с причиной. Путь к железу не трогается. Обхода нет |

«Железо есть» (`Initialize::hardwareDetected`) означает, что видна плата
Advantech из профиля (USB-4716/4718/4750) **или** порт ДВ301/ДВ302 из профиля.
Это строже, чем `!isInitializeOk()`: живая плата клапанов при недостающем
COM-порту запрещает демо. Виртуальные `DemoDevice` из DAQNavi к железу не
относятся.

Если на Windows нет `biodaq.dll`, `Initialize` больше не падает: библиотеку
сначала проверяет `LoadLibrary`, и только потом вызывается SDK.

## Схема

```mermaid
flowchart LR
    RPC["SimRpcServer<br/>QHttpServer 127.0.0.1"] --> D["SimRpcDispatcher"]
    D -->|"run/goto/pause/resume/stop"| C["SimController<br/>QStateMachine на прогон"]
    C --> TE["TrackEngine + SimNoise"]
    C -->|"Па / °C"| SS["SimSensorSource<br/>(ISensorSource)"]
    SS -->|"В / °C / Торр"| DAQ["DataAcquisition::processEvents"]
    DAQ --> CD["ControllerData · DataCollection<br/>калибровка A·V+B"]
    CD --> Q["квартили · GUI · графики"]
    CD --> R["рецепты режимов"]
    R --> VC["ValveControl → Security"]
    VC --> VE["SimValveEcho<br/>(IDoPort)"]
    VE -->|"SimValveEvent"| C
    SS -->|"readTemperature = такт"| C
```

### Точка подмены

Подменяются драйверные члены `DataAcquisition`: интерфейс
`controllers/ISensorSource.h`, настоящая реализация `RealSensorSource`, демо —
`sim::SimSensorSource`. Выше этой границы путь данных тот же, что с железом:

| Уровень | Настоящие данные | Демо |
|---|---|---|
| Драйвер | `AdvantechBuff` (фильтр Калмана), `AdvantechAI`, `VacuumController` / `TurboVacuumController` | `SimSensorSource` |
| Единицы на выходе | В (4–20 мА на шунте R), °C, Торр + `Quality` | те же: обратный перевод в `hw/SensorInverse` |
| `DataAcquisition::processEvents` | без изменений | без изменений |
| `ControllerData::addValue` → бар / °C | калибровка `(A/R·1000)·V + B` | та же |
| Квартили, GUI, `GasLeakage`, рецепты | без изменений | без изменений |

Почему именно здесь:

- **Выше** (писать бары прямо в `ControllerData` или подменять лямбды рецептов)
  обходится калибровка и остаются пустыми 128-точечные буферы
  `runSupplyAction` / `fillLeakageRQ`. Лямбды к тому же служат швами тестов.
- **Ниже** (эмулировать плату или COM-порт) понадобились бы biodaq, поток SDK и
  двоичный протокол вакуумметра, а выигрыша по идентичности пути нет.

Детали, сохранённые как у настоящих приборов:

- давление видно со следующего опроса: `readPressure` обновляет секцию после
  чтения;
- вакуумметры выше 761 Торр отдают `OverRange` с 761;
- «отфильтрованные» напряжения подаются без прогона через фильтр Калмана:
  переходный процесс фильтра исказил бы форму профиля.

Такт машины — `SimSensorSource::readTemperature`: его `processEvents` вызывает
каждый опрос (500 мс), даже в режиме быстрого чтения давления. Отдельного
таймера нет.

### Клапаны

У клапанов нет датчиков положения: `ValveControl::confirmValve` перечитывает
регистр-защёлку выхода платы. Демо-порт `SimValveEcho` (`controllers/IDoPort.h`)
возвращает последнюю записанную маску, то есть семантика та же, и проверка
REQ-082 не ослабляется. Каждый изменившийся бит уходит в машину фронтом
(`SimValveEvent`). `Security` и `setValveFromAction` не менялись, интерлок
К176⇄К179 работает на демо-порту так же, как на плате.

## Машина состояний

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Active: sim.run (SimRunEvent)
    state Active {
        [*] --> start_phase
        start_phase --> next_phase: SimTickEvent — timeout / фронт клапана, по порядку профиля
        H: QHistoryState (deep)
    }
    Active --> Active: SimGotoEvent (reason=manual)
    Active --> Paused: SimPauseEvent
    Paused --> H: SimResumeEvent
    Active --> Fault: SimFaultEvent
    Paused --> Fault: SimFaultEvent
    Active --> Idle: SimStopEvent
    Paused --> Idle: SimStopEvent
    Fault --> Idle: SimStopEvent
```

- Машина `QStateMachine` строится **на каждый прогон**: граф фаз задаёт профиль,
  а менять состояния работающей машины небезопасно. `Idle` означает, что машины
  нет. Остановка ведёт в `QFinalState`, после чего машина удаляется.
- Команды RPC попадают в машину только через `postEvent`. Такт, фронт клапана,
  goto, пауза, продолжение, остановка и отказ — пользовательские `QEvent`
  (`core/SimEvents.h`).
- Отдельного `SimTimeoutEvent` нет. Таймаут и фронт клапана проверяются
  переходами фазы на одном `SimTickEvent`, в порядке профиля: так соблюдается
  правило «срабатывает первый подходящий». Фронт, пришедший между тактами,
  запоминается в фазе и сбрасывается при входе в новую фазу.
- На паузе время фазы стоит, значения удерживаются, шум заморожен. Продолжение
  через `QHistoryState` возвращает ту же фазу с тем же `elapsedSec`, без новой
  записи в `history`. `sim.goto` на паузе переходит в фазу и оставляет прогон на
  паузе.
- `sim.stop` оставляет каналы на последних значениях.
- Подклассов фаз по виду нет: `static`, `pumping`, `gasInlet` и `h2Inlet`
  отличаются только умолчанием интерполяции и правилами проверки (`SimProfile`),
  поведение в работе одинаковое.

## Хранилище: демо отдельно от эксперимента

В демо-режиме:

- журнал прогонов пишется в `data/sim/regime_log.db` (`RegimeTaskTree::setLogDatabasePath`);
- снимки объёмов читаются и пишутся в базу PostgreSQL **`gramstate_sim`**
  (`GramStateDB::setDatabaseName`). Если такой базы нет, GRAMs работает без неё,
  как и без PostgreSQL. Чтобы viewer видел демо-снимки, базу создают с той же
  таблицей `gramstate`.

Не разделены: CSV «Напуска» и «Натекания» (`data/supplyData`, `data/leakageData`)
и `data/GRAMs-log.txt`. Пути CSV зашиты в `SupplyPort` и `GasLeakage` (квартили),
а их правка вне рамок задачи; открытые вопросы перечислены в
[`rpc-contract.md`](rpc-contract.md#вопросы-к-контракту-и-открытые-вопросы).

## Код

| Файл | Что делает |
|---|---|
| `src/sim/core/TrackEngine`, `SimNoise` | Интерполяция step/linear/log/exp; шум Бокса–Мюллера поверх `mt19937_64`, одинаковый на MSVC и GCC |
| `src/sim/core/SimProfile` | Разбор и проверка `grams.sim.profile/1`, коды Issue |
| `src/sim/core/SimCatalog` | Каналы, клапаны и участки из профиля установки |
| `src/sim/core/SimController`, `SimStates`, `SimEvents`, `SimClock` | Машина фаз |
| `src/sim/rpc/SimRpcDispatcher`, `SimRpcServer` | JSON-RPC 2.0 через HTTP |
| `src/sim/hw/SimSensorSource`, `SimValveEcho`, `SensorInverse` | Подмена источника данных и порта клапанов |
| `src/sim/app/SimOptions`, `SimSubsystem` | Флаги, сборка подсистемы, мост для QML |
| `src/controllers/ISensorSource.h`, `RealSensorSource.h`, `IDoPort.h`, `RealDoPort.h` | Интерфейсы и настоящие реализации |
| `src/qml/sim/DemoDataBanner.qml` | Плашка «ДЕМО-ДАННЫЕ» (через `Loader` в `Main.qml`) |
| `tests/sim/profiles/` | Эталонные профили и фикстуры ошибок |
| `tools/sim_rpc_smoke.ps1` | Дымовая проверка RPC |

## Тесты

```powershell
Push-Location build; ctest --output-on-failure; Pop-Location
.\build\src\sim\tests\SimTests.exe --gtest_filter=SimMachine*
# интеграционный — из корня репозитория (нужна папка profile/)
.\build\src\sim\tests\SimIntegrationTests.exe
```

| Сюита | Что проверяет |
|---|---|
| `SimTests` | TrackEngine, SimNoise, все коды валидации на фикстурах, машина фаз (timeout, фронт, порядок, goto, пауза, остановка, отказ, эталонные профили), RPC через `QTcpSocket`, обратный перевод через настоящий `ControllerData`, `SimValveEcho`, `SimOptions` |
| `SimIntegrationTests` | Настоящие `DataAcquisition`, `ValveControl`, `Security` и рецепт «Вакуума» на демо-данных. `vacuum_fore_turbo` доходит до конца через переход на К179. На `vacuum_leak` рост ДВ302 вызывает откат на форвакуум (`turboFallback`), как требует ТЗ. Время ускорено часами, 1 такт рецепта (10 мс) равен 1 с; контекст рецепта собирает тест (решение Р2) |

## Проверено и не проверено

- Приложение с `--sim` на машине без плат: RPC отвечает, прогон идёт по тактам
  опроса, `tools/sim_rpc_smoke.ps1` проходит; без `--sim` порт не открыт.
- Плашка QML визуально не проверялась: в `claude-dev` `Main.qml` не грузится
  из-за `import com.grams.prototable 1.0` в `RegimeSetup.qml`. Незакоммиченная
  правка основной копии это исправляет.
- **Не проверено на стенде:** перенос драйверов за `ISensorSource` / `IDoPort`
  (коммиты `refactor(daq)` и `refactor(valves)`) меняет путь данных с железом.
  До слияния нужен прогон на стенде.
- `Security` не получает давлений (`setPressureMap` не реализован), поэтому
  пороги давления не срабатывают ни на демо-данных, ни на настоящих. Это
  существующий дефект, демо-режим его не исправляет.
