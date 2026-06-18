# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

**GRAMs** (Gas Reaction Automated Machine) — Qt/C++/QML application for controlling the GRAM50 installation, which implements the **volumetric Sieverts method** for studying hydrogen sorption and desorption in solid materials (hydride materials).

The installation measures the amount of absorbed/released hydrogen by precisely tracking pressure in calibrated volumes before and after gas contact with a sample.

The previous software version is **GramQt** (separate repository). GRAMs is the new software into which all automatic modes from GramQt are being ported and improved. **Current priority (summer 2025):** porting automatic modes, starting with the Vacuum mode.

**Reference material for verification:** LaNi₅ — a well-studied hydride with predictable PCI curves (sorption plateau ~2 bar at 25°C).

## Build

```bash
# Configure (from repo root)
cmake -B build -S .

# Build
cmake --build build --config Debug

# Build release
cmake --build build --config Release
```

The build profile is hard-coded in `src/main.cpp`: `const auto &curInitProfile = QString("GRAM50");`. On Linux, `libDAQ/` is linked for the DAQ hardware; on Windows, `biodaq` is excluded and `opengl32` is added.

## Tests

Tests live in `src/runtable/tests/` and use GoogleTest + Qt6::Test.

```bash
# Run all tests
cd build && ctest --output-on-failure

# Run a single test binary directly
./build/src/runtable/tests/ProtoTableTests --gtest_filter=*RegimeManager*
```

GoogleTest is fetched automatically via `FetchContent` on first configure.

## Physical model of the installation

### Gas circuit as a graph

The installation is modelled as an **oriented graph**: nodes are volumes (reservoirs), edges are valves and needle valves. System state = set of pressures in nodes + edge states (open/closed).

### Nodes (reservoirs)

| Label | Purpose |
|---|---|
| B | Buffer volume (main gas inlet reservoir) |
| E | Volume from pressure gauge to ± valve |
| F | Gas supply volume |
| D1 | Dosing volume 1 |
| D2 | Dosing volume 2 |
| C1, C2, C3 | Calibrated volumes ("vessels") |
| Chamber | Reaction chamber with sample |

### Physical formulas

**Flow regime criterion for H₂** (γ = 1.4): subsonic flow when P_out/P_in > 0.528; sonic (critical) flow when P_out/P_in ≤ 0.528.

**Hydrogen quantity** is calculated via the ideal gas equation of state. The compressibility factor for hydrogen at high pressures is accounted for separately.

**Flow model through a needle valve** (Swagelok formulas): subsonic mode — flow depends on both pressures; sonic mode — flow depends only on inlet pressure.

**Kalman filter** is applied to pressure sensor readings. Filter parameters are configured through an external JSON file in `profile/`.

## Automatic modes

All modes are implemented in GramQt and are being ported to GRAMs:

| Mode | Description |
|---|---|
| **Vacuum** | Pumping the system to a target pressure with a vacuum pump |
| **SYSTEST** | Comprehensive check: Calibration + Vacuum |
| **SOAK** | Sample hydrogenation: hold at target H₂ pressure |
| **PCI** | Pressure-Composition Isotherm construction |
| **Inlet (Напуск)** | Fast gas supply to target pressure in B |
| **Leakage (Натекание)** | Slow gas supply through needle valve into chamber |
| **Calibration** | Determination of volumes A–B by expansion method |

## Architecture

### Application root: `Grams` class

`src/Grams.h` / `src/Grams.cpp` — the central class, subclasses `QApplication`. It owns all domain objects and wires them together during construction via a series of `init*()` private methods (`initDigitalData`, `initAnalogData`, `initCharts`, `initSafeModule`, `initActionHandler`, etc.). The main poll loop runs on `softTimer` → `softEvent()`.

### Hardware profile: `Initialize`

`src/Initialize.h` — reads a JSON profile (e.g. `GRAM50`) from `profile/` and populates typed parameter structs (`daqParameters`, `hardwareParameters`, `storageQuarParameters`, `reactionQuarParameters`, `securityParameters`, `vacuumParameters`). These structs are `Q_GADGET` so they are directly accessible from QML. `Grams` passes these structs to each subsystem during `init*()`.

### Data acquisition: `DataAcquisition`

`src/DataAcquisition.h` — drives Advantech DAQ boards (`AdvantechBuff` for AI pressure, `AdvantechAI` for AI temperature) and a serial vacuum gauge (`VacuumController`). Data is stored through raw pointers to `ControllerData` / `FilterData` / `DataCollection` objects that live in `Grams`. Kalman filter post-processing runs inside `FilterView` (`src/FilterView.h`).

### Data types hierarchy (`src/DataCollection.h`)

```
DataCollection          — named ring buffer, altUnit coefficient
  ControllerData        — adds linear calibration (A·x + B) for raw sensor voltage
  FilterData            — adds cumulative buffer for Kalman input
  QuartileData          — pressure snapshot for a gas volume
  MolesData             — mole-count accumulator per volume
  NodeData              — pressure node (equilibrium calculation)
```

### Valve control: `ValveControl`

`src/ValveControl.h` — writes digital output states to `AdvantechDO`. All writes go through `Security` first (`checkOpenChamber`, contradiction checks). Accepts `Valve*` pointer arrays from `Grams`; the physical valve list is in `Grams.h` (`vAR1`…`vR5`). `ControlDO` is a separate object for discrete output control, passed into `ActionHandler` explicitly.

### Pressure domain: Quartile subsystem (`src/addon/`)

The gas circuit graph is divided into four zones, each modelled by a `Quartile` subclass holding `VolumeObject` maps, `NodePressure` equilibrium nodes, and pointers to the relevant `ControllerData` / `Valve`:

| Class | Zone |
|---|---|
| `AddRemoveQuartile` | Supply / drain side (inlet gas) |
| `StorageQuartile` | High-pressure storage vessels (C1, C2, C3, B) |
| `ReactionQuartile` | Reaction chamber area |
| `SecondLineQuartile` | Second gas line / mass spectrometer |

`NodePressure` — the physical model of a graph node. Stores pressure in a volume, knows how to do `split` (volume separation when a valve opens) and `collapse` (unification when a valve closes). This is the central abstraction — do not change the architecture without understanding the physics.

`QuartileManager` aggregates all quartiles and is the single entry point for pressure redistribution queries.

`StrategyBuilder` (`src/addon/StrategyBuilder.h`) constructs `SupplyPort` / `GasLeakage` strategy objects from quartile state. Strategy classes (e.g. `InletStrategy`, `StoragePressureStrategy`) contain mode logic and are separated from GUI. GUI structures intentionally live in quartile — this is an architectural decision.

### Async actions: `ActionHandler` + `InletAction` / `DataControlAction`

`src/actions/ActionHandler.h` — the entry point for all automatic modes. Uses `QPromise` / `QFutureWatcher<int>` to run timed valve sequences off the main thread. A mode = sequence of Action steps, each working with a strategy. `InletAction` models a single gas-inlet event (open valve for duration); `DataControlAction` controls the DAQ during an action. `ActionHandler` gets pointers to `ValveControl`, `DataAcquisition`, and `Security` injected by `Grams`.

### Experiment scheduler: `RegimeManager` + `RuntableLib`

`src/runtable/` is compiled as a separate QML module (`com.grams.prototable`, URI registered as `RuntableLib`). It exposes:

- `ProtoTableModel` — list model of `Regime` objects (editable via QML `RunTable.qml`)
- `RegimeManager` — owns `ProtoTableModel` and `VisibleRegimeModel`; driven externally via `Q_INVOKABLE` API (`startRegimeExecution`, `updateConditionProgress`, `completeCurrentRepeat`, …)
- `VisibleRegimeModel` — filtered/windowed view of regimes for timeline rendering

Regime state transitions: `Idle → Running (condition phase) → Running (execution phase) → Done / Skipped / Error`.

### Persistence: `GramStateDB` + `TimeStamp`

`src/db/GramStateDB.h` — SQLite database (Qt6::Sql) that saves vessel pressure states on valve changes and on program close. Also saves start/end of each Action and experiment parameters; fast sensor readings go to CSV (`fastResult`), organized by experiment folder + index.

`src/TimeStamp.h` — in-memory snapshot of the last known physical state; used to seed virtual volume calculations after restart. Every change to vessel and chamber volumes is placed into `TimeStamp`.

### QML UI (`src/qml/`)

Entry point: `src/qml/Main.qml`. Navigation is done through `SideMenu.qml`. Main functional areas:

- `lumber/Lumber.qml` — main process view with sub-views for supply (`LSupply`), storage (`LStorage`), reaction (`LReaction`), vacuum (`LVacuum`), chamber (`LChamber`), furnace (`LFurnace`)
- `mnemo/GRAM50_mnemo/` and `mnemo/GRAM300_mnemo/` — schematic mnemonic diagrams
- `charts/GraphWindow.qml`, `ChartWindow.qml` — QCustomPlot-backed chart windows (OpenGL enabled via `QCUSTOMPLOT_USE_OPENGL`)
- `measure/` — manual measurement panels
- `qml/lumber/PlayTable.qml` — connects to `RegimeManager` for run-table display

`Grams` is registered as a context property `app`; QML accesses sensor values through `app.guiPres`, `app.guiTemp`, `app.guiPresVirtual` and valve states through `app.guiValve`.

### Hardware

| Device | Purpose |
|---|---|
| Advantech USB-4750 | Digital I/O (valve control) |
| Advantech USB-4716 | Analog input (pressure sensors) |
| Advantech USB-4718 | Analog input (thermocouples) |
| Vacuum gauge (serial) | Pressure measurement during pumping |

Devices are initialized in `Initialize`; controller objects live in `Controller`. Optional devices are handled separately through configuration.

### Logging

`main.cpp` installs a custom message handler (`myMessageHandler`) that writes timestamped log lines to `data/GRAMs-log.txt` relative to the working directory. The `data/` folder is created automatically on startup.

## Naming conventions

```
m_      — class members (m_pressure, m_valveState)
v_      — virtual objects (v_pressureRange, v_safeModule)
ptr[]   — pointer arrays — always check for nullptr before use
```

## What must not be changed without explicit request

- The `Quartile` singleton architecture
- The `split` / `collapse` logic in `NodePressure` — it is physically verified
- The pattern of passing `ControlDO` into `ActionHandler`
- Naming conventions (prefixes `m_`, `v_`, `ptr[]`)
