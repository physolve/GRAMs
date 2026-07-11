# ValveTestSetup

> AI assistance has been used to create this output.

## 1. Component Overview

GRAMs is a Qt Quick application that automates the GRAM50 volumetric gas-sorption installation (Sieverts method). The UI is organized as a side-panel navigation: each "page" is a distinct full-size item loaded inside `SideMenu.qml`.

`ValveTestSetup` is the configuration and control page for the **Тест клапанов** (valve test) regime. It lets the operator:

1. Define an ordered sequence of test **steps**, each specifying which physical valves open together and for how long.
2. Configure global repetition count and inter-run pauses.
3. Push the configuration to the C++ backend and start/pause/stop the regime — all without leaving the page.

The component is a single self-contained page: it owns the `ListModel` that holds the step sequence, serialises it into `QVariantList`, and drives `RegimeTaskTree` directly.

---

## 2. Project Structure and Dependencies

**Instantiated by:** `src/qml/SideMenu.qml` (line 146) — fills the entire content area via `Layout.fillWidth/fillHeight`.

**Qt modules imported:**
- `QtQuick` — base types, `Item`, `Rectangle`, `Text`, `Flow`, `Column`, `Row`, `Repeater`, `ListModel`
- `QtQuick.Controls` — `SpinBox`, `ComboBox`, `Button`, `ScrollView`, `ScrollBar`
- `QtQuick.Controls.Material` — Material styling; used for `Material.background` on action buttons
- `QtQuick.Layouts` — `ColumnLayout`, `RowLayout`, `GridLayout`

**Custom imports:**
- `Grams.regimeTaskTreeSingleton 1.0` — exposes the `RegimeTaskTree` C++ singleton (registered in `Grams.cpp`). Provides `availableValves`, `running`, `paused`, and all control methods.
- `"lumber/content"` (relative) — provides the `ScrollArrow` component used as custom SpinBox up/down indicators.

**C++ backend — `RegimeTaskTree`** (`src/actions/RegimeTaskTree.h`):

| Property / Method | Type | Description |
|---|---|---|
| `availableValves` | `QStringList` (CONSTANT) | Physical valve names seeded from `Grams::initActionHandler()` |
| `running` | `bool` (NOTIFY runningChanged) | True while a regime execution is in progress |
| `paused` | `bool` (NOTIFY pausedChanged) | True while execution is paused |
| `setValveTestSteps(steps, repeats, pauseBefore, pauseAfter)` | `Q_INVOKABLE` | Pushes the serialised step list to the worker |
| `startAll()` | slot | Starts all Waiting regimes in order |
| `pause()` / `resume()` | slots | Pause/resume active execution |
| `stop()` | slot | Cancels execution, marks regimes Stopped |

---

## 3. Component Hierarchy and Role

`ValveTestSetup` inherits `Item`. It is a full-page layout component — not a reusable widget — so it anchors to its parent and delegates all sizing to the layout system via `Layout.fillWidth/fillHeight`.

Internally the layout is a three-row `ColumnLayout`:
- **Global params bar** (fixed height 82 px) — repeat count + global pre/post pauses.
- **Step list** (`ScrollView`, fills remaining height) — a `Repeater` over `stepsModel`, one card per step.
- **Control bar** (fixed height 48 px) — Apply / Start / Pause / Stop buttons.

---

## 4. Properties

### Internal palette (read-only)

All colour properties are `readonly` and private to the component; they are not settable by the parent.

| Property | Type | Default | Description |
|---|---|---|---|
| `cBg` | `color` | `#464646` | Root background (currently unused in layout) |
| `cCard` | `color` | `#3A3A3A` | Step card and bar background |
| `cBorder` | `color` | `#ABDBDD` | Default step border (idle state) |
| `cAccent` | `color` | `#594E74` | Accent border for the global-params and control bars |
| `cRunning` | `color` | `#4CAF50` | Step border colour while regime is running |
| `cText` | `color` | `#FFFFFF` | Primary text |
| `cSub` | `color` | `#9090A0` | Secondary / label text |
| `cValveBg` | `color` | `#2A2A2A` | Background of individual valve squares |
| `cDanger` | `color` | `#8B3030` | Stop button and per-valve delete button accent |
| `cMerge` | `color` | `#2D5A7A` | Merge-step button accent |

### Editable state

| Property | Type | Default | Description |
|---|---|---|---|
| `globalRepeats` | `int` | `1` | Number of times the full step sequence is repeated. Range 1–99. Updated by the SpinBox; flushed to C++ on Apply. |
| `globalPauseBefore` | `int` | `0` | Pause (seconds) before the first step of each run. Range 0–600. |
| `globalPauseAfter` | `int` | `0` | Pause (seconds) after the last step of each run. Range 0–600. |

### Internal model

`stepsModel` is a `ListModel` (private, `id`-only). Each row stores:

| Role | Type | Default | Description |
|---|---|---|---|
| `valves` | `string` | one valve name | Comma-separated list of valve names opened simultaneously in this step |
| `pauseBefore` | `int` | `0` | Per-step pre-open pause (seconds). Range 0–600. |
| `dwell` | `int` | `5` | Duration to hold the valve(s) open (seconds). Range 1–600. |
| `pauseAfter` | `int` | `0` | Per-step post-close pause (seconds). Range 0–600. |

---

## 5. Signals

`ValveTestSetup` declares no custom signals. All output is via direct invocable calls on `RegimeTaskTree`.

---

## 6. Methods

#### valveList(int idx) : Array

Returns the list of valve names at step `idx` as a JavaScript `Array<string>`. Splits the comma-separated `valves` role. Returns an empty array for out-of-range indices.

#### setValves(int idx, Array list)

Writes a new valve-name array back to `stepsModel` row `idx` as a comma-joined string, triggering reactive re-evaluation of the valve Repeater inside the step delegate.

#### removeValveFromStep(int stepIdx, string name)

Removes valve `name` from step `stepIdx`. If removing it leaves the step with zero valves, the entire step row is deleted from `stepsModel`.

#### addValveToStep(int stepIdx, string name)

Appends valve `name` to step `stepIdx`. No-ops silently if `name` is empty or already present in that step.

#### moveUp(int idx)

Moves step `idx` one position earlier in `stepsModel`. No-ops if `idx` is 0.

#### moveDown(int idx)

Moves step `idx` one position later in `stepsModel`. No-ops if `idx` is the last step.

#### mergeWithPrev(int idx)

Merges step `idx` into the preceding step by concatenating their `valves` strings. The row at `idx` is then deleted. All merged valves open simultaneously in the surviving step. No-ops if `idx` is 0.

#### mergeWithNext(int idx)

Merges step `idx` with the following step. The following row is deleted; merged valves are stored in step `idx`. No-ops if `idx` is the last step.

#### applyConfig()

Serialises the full `stepsModel` into a `QVariantList` of `QVariantMap` objects and calls `RegimeTaskTree.setValveTestSteps(steps, globalRepeats, globalPauseBefore, globalPauseAfter)`. Each map has keys `valves` (`QStringList`), `pauseBefore`, `dwell`, `pauseAfter` (all `int`). This is the single point of truth push to the C++ worker; it must be called before `startAll()`, and the **Старт** button calls it automatically.

---

## 7. Inter-Component Interactions

**`RegimeTaskTree` singleton (read)**
- `RegimeTaskTree.availableValves` — populates `stepsModel` on `Component.onCompleted` (one initial step per valve) and fills the per-step `ComboBox` picker.
- `RegimeTaskTree.running` — disables the **Старт** button; also switches step-card border colour from `cBorder` to `cRunning`.
- `RegimeTaskTree.paused` — toggles the Pause/Resume button label.

**`RegimeTaskTree` singleton (write)**
- `RegimeTaskTree.setValveTestSteps(...)` — called by `applyConfig()`.
- `RegimeTaskTree.startAll()` — called by **Старт** (after `applyConfig()`).
- `RegimeTaskTree.pause()` / `RegimeTaskTree.resume()` — called by the Pause/Resume toggle button.
- `RegimeTaskTree.stop()` — called by **Стоп**.

**`SideMenu.qml`** instantiates `ValveTestSetup` directly with no property bindings beyond layout constraints. No signals flow upward.

---

## 8. Usage Example

`ValveTestSetup` has no `required` properties; callers need only allocate layout space:

```qml
import QtQuick.Layouts

// inside a StackLayout or ColumnLayout page host:
ValveTestSetup {
    Layout.fillWidth: true
    Layout.fillHeight: true
}
```

`RegimeTaskTree` must be registered as a QML singleton before this page is loaded (done in `Grams::initActionHandler()`), and `setValveNamesForTest()` must be called beforehand so `availableValves` is non-empty.
