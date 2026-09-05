import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.regimeTaskTreeSingleton 1.0 // RegimeTaskTree + RegimeTaskTree.vacuumMonitor

// ─────────────────────────────────────────────────────────────────────────────
//  RegimeApiSandbox — живая развёртка рецепта выбранного режима (сейчас «Вакуум»,
//  VacuumTaskTree) с непосредственным отражением состояния выполнения каждого узла.
//
//  По образцу Qt TaskTree demo (C:/Qt/Examples/Qt-6.11.1/tasktree/demo): каждый
//  узел рецепта инструментирован onGroupSetup/onGroupDone → состояние пушится в
//  VacuumRunMonitor (C++), а страница только отражает его.
//
//  Данные: RegimeTaskTree.vacuumMonitor (steps-модель + скалярные свойства).
//  В дальнейшем логика переедет в RegimeManager.
// ─────────────────────────────────────────────────────────────────────────────
Item {
    id: root

    // ── Палитра ───────────────────────────────────────────────────────────────
    readonly property color cBg:      "#464646"
    readonly property color cCard:    "#3A3A3A"
    readonly property color cBorder:  "#ABDBDD"
    readonly property color cAccent:  "#594E74"
    readonly property color cText:    "#FFFFFF"
    readonly property color cSub:     "#9090A0"
    readonly property color cField:   "#2A2A2A"

    // ── Цвета состояний узла (схема demo: gray/yellow/green/red/cyan) ──────────
    // NodeState: 0 Initial, 1 Running, 2 Success, 3 Error, 4 Cancelled, 5 Skipped
    function stateColor(s) {
        switch (s) {
        case 1:  return "#E6C33A"   // Running — жёлтый
        case 2:  return "#4CAF50"   // Success — зелёный
        case 3:  return "#C0392B"   // Error   — красный
        case 4:  return "#3AA6B9"   // Cancelled — cyan
        case 5:  return "#5A5A5A"   // Skipped — тусклый
        default: return "#808080"   // Initial — серый
        }
    }
    function stateText(s) {
        switch (s) {
        case 1:  return "Running"
        case 2:  return "Success"
        case 3:  return "Error"
        case 4:  return "Cancelled"
        case 5:  return "Skipped"
        default: return "—"
        }
    }

    // Наблюдатель рецепта «Вакуум» (C++ VacuumRunMonitor)
    readonly property var mon: RegimeTaskTree.vacuumMonitor

    // Клапаны в стабильном порядке (метки DO-каналов GRAMs)
    readonly property var valveOrder: ["AR4", "S3", "S1", "S2", "SL2", "SL1", "AR6", "AR5", "R3"]

    Rectangle { anchors.fill: parent; color: cBg }

    // ── Операторский диалог dP/dt (OperatorBus: Стоп / Продолжить) ─────────────
    Connections {
        target: RegimeTaskTree.operatorBus
        function onDecisionRequired(code, message) {
            operatorDialog.message = message
            operatorDialog.open()
        }
        function onDecisionReceived(d) {
            operatorDialog.close()
        }
    }

    Dialog {
        id: operatorDialog
        property string message: ""
        anchors.centerIn: Overlay.overlay
        modal: true
        closePolicy: Popup.NoAutoClose
        title: "Проверка откачки"
        standardButtons: Dialog.NoButton
        // Ширина задана явно: иначе implicitWidth диалога считается от
        // переносимого текста, а тот — от ширины диалога (binding loop).
        width: 420

        contentItem: ColumnLayout {
            spacing: 16
            Label {
                text: operatorDialog.message
                color: root.cText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                Button {
                    text: "⏹ Остановить"
                    onClicked: RegimeTaskTree.operatorBus.respond(2)  // Stop → ошибка → стоп режима
                }
                Button {
                    text: "▷ Продолжить"
                    onClicked: RegimeTaskTree.operatorBus.respond(1)  // Continue → повтор проверки
                }
            }
        }
    }

    // ── Опции запуска (пушатся в C++ на «Старт») ──────────────────────────────
    property bool skipRK10:    false
    property bool skipRK50:    false
    property bool skipRK300:   false
    property bool secondTract: false
    property bool foreVacuum:  true   // форвакуумная откачка 11.5–11.7
    property bool pumpCheck:   true   // dP/dt-watchdog после открытия К176
    property int  repeats:     1
    // Тракт остаётся открытым после успешного прогона — насос качает дальше.
    property bool continuousPumping: false

    // Длительности зафиксированы в C++ (VacuumTreeContext) и из UI не правятся:
    // шаг 3 с, сброс К118 10 с, «мёртвая зона» dP/dt 30 с.
    readonly property string fixedTimings: "шаг 3 с · сброс К118 10 с · dP/dt через 30 с"

    // Цель форвакуума 11.5–11.7 (ТЗ REQ-022). targetVacPa вводится текстом, а не
    // SpinBox: диапазон 1e-4…1e5 Па охватывает пять порядков.
    property real targetVacPa:  40.0
    property int  holdSec:      60
    property int  foreVacTimeoutSec: 300

    // «Должно быть» для строки 11.x — из монитора, чтобы UI показывал реально
    // применённое значение (после qBound в C++), а не то, что набрано в поле.
    function fmtPa(v) {
        if (!isFinite(v)) return "—"
        if (v >= 1000 || (v > 0 && v < 0.01)) return v.toExponential(2) + " Па"
        return v.toFixed(2) + " Па"
    }

    component Card : Rectangle {
        default property alias cardData: inner.data
        Layout.fillWidth: true
        color: root.cCard
        radius: 6
        border.color: root.cBorder
        border.width: 1
        implicitHeight: inner.implicitHeight + 14
        ColumnLayout {
            id: inner
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 7 }
            spacing: 5
        }
    }

    // Компактные варианты контролов: единый мелкий шрифт и минимальные отступы,
    // чтобы вся панель настроек умещалась в две строки.
    component Chk : CheckBox {
        padding: 2
        font.pointSize: 9
        ToolTip.visible: hovered && ToolTip.text.length > 0
        ToolTip.delay: 500
    }
    component Num : SpinBox {
        font.pointSize: 9
        padding: 2
        Layout.preferredWidth: 104
    }
    component Cap : Label {
        color: root.cSub
        font.pointSize: 9
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.margins: 8
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 8

            // ── Заголовок + селектор режима ────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Развёртка рецепта режима"
                    color: root.cText
                    font.pointSize: 13
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Cap { text: "Режим:" }
                ComboBox {
                    id: regimeSelector
                    model: ["Вакуум"]     // задел под будущие режимы
                    font.pointSize: 9
                    Layout.preferredWidth: 140
                }
            }

            // ── Опции запуска + управление (одна компактная карточка) ─────────
            Card {
                // Ряд 1 — что откачиваем.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Cap { text: "Пропуск C:" }
                    // Метки в терминах интерфейса C1/C2/C3 (ТЗ REQ-005);
                    // легаси-имена RK и DO-каналы — в подсказках.
                    Chk {
                        text: "C1"; ToolTip.text: "RK300 / S3 (К135)"
                        checked: root.skipRK300; onToggled: root.skipRK300 = checked
                    }
                    Chk {
                        text: "C2"; ToolTip.text: "RK50 / S2 (К133)"
                        checked: root.skipRK50;  onToggled: root.skipRK50 = checked
                    }
                    Chk {
                        text: "C3"; ToolTip.text: "RK10 / S1 (К131)"
                        checked: root.skipRK10;  onToggled: root.skipRK10 = checked
                    }
                    Rectangle {
                        implicitWidth: 1; Layout.preferredHeight: 18
                        color: root.cBorder; opacity: 0.4
                    }
                    Chk {
                        text: "2-й тракт"; ToolTip.text: "Ф3: К192/К179 (s20–s24)"
                        checked: root.secondTract; onToggled: root.secondTract = checked
                    }
                    Chk {
                        text: "форвакуум"; ToolTip.text: "Этапы 11.5–11.7 через К176"
                        checked: root.foreVacuum; onToggled: root.foreVacuum = checked
                    }
                    Chk {
                        text: "dP/dt"; ToolTip.text: "Проверка скорости откачки после открытия К176"
                        checked: root.pumpCheck; onToggled: root.pumpCheck = checked
                    }
                    Chk {
                        text: "непрерывная откачка"
                        ToolTip.text: "После успешного прогона оставить тракт открытым — насос качает дальше"
                        checked: root.continuousPumping
                        onToggled: root.continuousPumping = checked
                    }
                    Item { Layout.fillWidth: true }
                    Cap { text: "Повторы:" }
                    Num { from: 1; to: 99; value: root.repeats; onValueModified: root.repeats = value }
                }

                // Ряд 2 — цель форвакуума 11.5–11.7 (ТЗ REQ-022). targetVacPa
                // вводится текстом, а не SpinBox: диапазон 1e-4…1e5 Па.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    enabled: root.foreVacuum
                    opacity: enabled ? 1.0 : 0.45
                    Cap { text: "Форвакуум до:" }
                    TextField {
                        id: targetField
                        Layout.preferredWidth: 76
                        text: root.targetVacPa
                        font.pointSize: 9
                        font.family: "Consolas"
                        horizontalAlignment: TextInput.AlignRight
                        validator: DoubleValidator { bottom: 0.0001; top: 100000; notation: DoubleValidator.ScientificNotation }
                        onEditingFinished: {
                            var v = parseFloat(text)
                            if (!isNaN(v) && v > 0) root.targetVacPa = v
                            else text = root.targetVacPa
                        }
                    }
                    Cap { text: "Па (ДВ301), удержать" }
                    Num {
                        from: 1; to: 3600; stepSize: 5
                        value: root.holdSec; onValueModified: root.holdSec = value
                    }
                    Cap { text: "с, таймаут" }
                    Num {
                        from: 10; to: 7200; stepSize: 30
                        value: root.foreVacTimeoutSec
                        onValueModified: root.foreVacTimeoutSec = value
                    }
                    Cap { text: "с" }
                    Button {
                        text: "⟲"
                        flat: true
                        font.pointSize: 9
                        ToolTip.text: "Значения по умолчанию: 40 Па / 60 с / 300 с"
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        onClicked: {
                            root.targetVacPa = 40.0
                            root.holdSec = 60
                            root.foreVacTimeoutSec = 300
                            targetField.text = root.targetVacPa
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                // Ряд 3 — управление прогоном. Длительности шагов не настраиваются:
                // они зафиксированы в VacuumTreeContext, здесь только напоминание.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Button {
                        text: "▶ Старт"
                        font.pointSize: 9
                        enabled: !RegimeTaskTree.running
                        onClicked: {
                            RegimeTaskTree.setVacuumOptions(root.skipRK10, root.skipRK50,
                                                            root.skipRK300, root.secondTract)
                            RegimeTaskTree.setVacuumForevac(root.foreVacuum)
                            RegimeTaskTree.setVacuumForevacTarget(root.targetVacPa,
                                                                  root.holdSec,
                                                                  root.foreVacTimeoutSec)
                            RegimeTaskTree.setVacuumPumpCheck(root.pumpCheck)
                            RegimeTaskTree.setVacuumContinuousPumping(root.continuousPumping)
                            RegimeTaskTree.startAll()
                        }
                    }
                    Button {
                        text: RegimeTaskTree.paused ? "▷ Резюм" : "⏸ Пауза"
                        font.pointSize: 9
                        enabled: RegimeTaskTree.running
                        onClicked: RegimeTaskTree.paused ? RegimeTaskTree.resume()
                                                         : RegimeTaskTree.pause()
                    }
                    Button {
                        text: "⏹ Стоп"
                        font.pointSize: 9
                        enabled: RegimeTaskTree.running
                        onClicked: RegimeTaskTree.stop()
                    }
                    Button {
                        text: "⟲ Сброс"
                        font.pointSize: 9
                        enabled: !RegimeTaskTree.running
                        onClicked: root.mon.reset()
                    }
                    Cap { text: root.fixedTimings; font.pointSize: 8 }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: RegimeTaskTree.running
                              ? (RegimeTaskTree.paused ? "на паузе" : "выполняется")
                              : "остановлен"
                        color: RegimeTaskTree.running ? root.stateColor(1) : root.cSub
                        font.pointSize: 9
                        font.bold: true
                    }
                }
            }


            // ── Развёртка рецепта (дерево узлов + состояния) ───────────────────
            Card {
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Рецепт «Вакуум» — узлы и состояние"
                        color: root.cBorder; font.bold: true; font.pointSize: 10
                    }
                    Item { Layout.fillWidth: true }
                    Cap { text: "текущая фаза: " + (root.mon.currentLabel.length ? root.mon.currentLabel : "—") }
                }
                Repeater {
                    model: root.mon.steps
                    delegate: ColumnLayout {
                        id: stepRow
                        required property var model
                        // Узлы форвакуума (VacuumNode: F5A1=8, F6BC=9, F7EF=10) —
                        // единственные, под которыми рисуется цель и её прогресс.
                        readonly property bool isForevac: stepRow.model.nodeId >= 8
                                                          && stepRow.model.nodeId <= 10
                        readonly property bool pumping: root.mon.forevacActive
                                                        && root.mon.forevacNode === stepRow.model.nodeId
                        Layout.fillWidth: true
                        spacing: 2

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 26
                            color: stepRow.model.depth > 0 ? root.cField : "transparent"
                            radius: 4
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8 + stepRow.model.depth * 26
                                anchors.rightMargin: 8
                                spacing: 10
                                // Чип состояния
                                Rectangle {
                                    // implicit*, а не width/height: элемент под управлением RowLayout
                                    implicitWidth: 14; implicitHeight: 14; radius: 3
                                    color: root.stateColor(stepRow.model.state)
                                    border.color: root.cBg; border.width: 1
                                    // «Спиннер»: пульсация при Running
                                    SequentialAnimation on opacity {
                                        running: stepRow.model.state === 1
                                        loops: Animation.Infinite
                                        NumberAnimation { to: 0.35; duration: 500 }
                                        NumberAnimation { to: 1.0;  duration: 500 }
                                    }
                                }
                                Label {
                                    text: (stepRow.model.phase.length ? stepRow.model.phase + "  " : "")
                                          + stepRow.model.label
                                    color: root.cText
                                    font.bold: stepRow.model.depth === 0
                                }
                                Label {
                                    text: stepRow.model.sRange
                                    color: root.cSub; font.pointSize: 8; font.family: "Consolas"
                                }
                                Item { Layout.fillWidth: true }
                                Label {
                                    text: root.stateText(stepRow.model.state)
                                    color: root.stateColor(stepRow.model.state)
                                    font.pointSize: 9
                                }
                            }
                        }

                        // ── Прогресс откачки под строкой узла 11.5 / 11.6 / 11.7 ──
                        // Цель («должно быть») видна всегда; текущее ДВ301, набранное
                        // удержание и таймаут — только пока качает именно этот узел.
                        Rectangle {
                            visible: stepRow.isForevac
                            Layout.fillWidth: true
                            Layout.leftMargin: 8 + stepRow.model.depth * 26 + 24
                            implicitHeight: fvCol.implicitHeight + 12
                            color: stepRow.pumping ? Qt.darker(root.cAccent, 1.15) : root.cField
                            radius: 4
                            border.width: stepRow.pumping ? 1 : 0
                            border.color: root.cBorder

                            ColumnLayout {
                                id: fvCol
                                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 6 }
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Label {
                                        text: "цель: ≤ " + root.fmtPa(root.mon.forevacTargetPa)
                                        color: root.cBorder
                                        font.pointSize: 9; font.bold: true
                                        font.family: "Consolas"
                                    }
                                    Label {
                                        text: "удержание " + root.mon.forevacHoldSec
                                              + " с · таймаут " + root.mon.forevacTimeoutSec + " с"
                                        color: root.cSub; font.pointSize: 8
                                    }
                                    Item { Layout.fillWidth: true }
                                    Label {
                                        visible: stepRow.pumping
                                        text: root.mon.forevacHasReading
                                              ? "ДВ301: " + root.fmtPa(root.mon.forevacCurrentPa)
                                              : "ДВ301: нет датчика"
                                        color: !root.mon.forevacHasReading ? root.stateColor(3)
                                             : (root.mon.forevacCurrentPa <= root.mon.forevacTargetPa
                                                ? root.stateColor(2) : root.cText)
                                        font.pointSize: 10; font.bold: true
                                        font.family: "Consolas"
                                    }
                                }

                                // Набранное непрерывное удержание ≤ цели
                                RowLayout {
                                    visible: stepRow.pumping
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Label { text: "удержано:"; color: root.cSub; font.pointSize: 8 }
                                    ProgressBar {
                                        Layout.fillWidth: true
                                        from: 0
                                        to: Math.max(1, root.mon.forevacHoldSec)
                                        value: root.mon.forevacHeldSec
                                    }
                                    Label {
                                        text: root.mon.forevacHeldSec + " / " + root.mon.forevacHoldSec + " с"
                                        color: root.cText; font.pointSize: 8; font.family: "Consolas"
                                    }
                                }

                                // Время этапа против таймаута
                                RowLayout {
                                    visible: stepRow.pumping
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Label { text: "этап:"; color: root.cSub; font.pointSize: 8 }
                                    ProgressBar {
                                        Layout.fillWidth: true
                                        from: 0
                                        to: Math.max(1, root.mon.forevacTimeoutSec)
                                        value: root.mon.forevacElapsedSec
                                    }
                                    Label {
                                        text: root.mon.forevacElapsedSec + " / "
                                              + root.mon.forevacTimeoutSec + " с"
                                        color: root.mon.forevacElapsedSec > root.mon.forevacTimeoutSec * 0.8
                                               ? root.stateColor(1) : root.cSub
                                        font.pointSize: 8; font.family: "Consolas"
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Live-состояние: прогресс, повтор, клапаны ──────────────────────
            Card {
                Label { text: "Состояние выполнения"; color: root.cBorder; font.bold: true }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: 20
                    rowSpacing: 6
                    Label { text: "Повтор:"; color: root.cSub }
                    Label { text: root.mon.currentRepeat + " / " + root.mon.totalRepeats; color: root.cText }
                    Label { text: "Выдержка, с:"; color: root.cSub }
                    Label { text: root.mon.elapsedSec; color: root.cText }
                    Label { text: "Готово повторов:"; color: root.cSub }
                    Label { text: root.mon.repeatsDone; color: root.stateColor(2) }
                    Label { text: "С ошибкой:"; color: root.cSub }
                    Label { text: root.mon.repeatsError; color: root.stateColor(3) }
                }

                // ── Причина завершения режима ─────────────────────────────────
                // RegimeEnums::State: 1 Stopped, 5 Done, 6 Error.
                Rectangle {
                    visible: root.mon.finishReason.length > 0
                    Layout.fillWidth: true
                    implicitHeight: finishRow.implicitHeight + 16
                    radius: 4
                    color: root.cField
                    border.width: 1
                    border.color: root.mon.finishState === 6 ? root.stateColor(3)
                                : root.mon.finishState === 5 ? root.stateColor(2)
                                                             : root.stateColor(4)
                    RowLayout {
                        id: finishRow
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 8 }
                        spacing: 10
                        Label {
                            text: root.mon.finishState === 6 ? "✕"
                                : root.mon.finishState === 5 ? "✓" : "⏹"
                            color: root.mon.finishState === 6 ? root.stateColor(3)
                                 : root.mon.finishState === 5 ? root.stateColor(2)
                                                              : root.stateColor(4)
                            font.pointSize: 13; font.bold: true
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: root.mon.finishState === 6 ? "Режим завершён с ошибкой"
                                    : root.mon.finishState === 5 ? "Режим завершён"
                                                                 : "Режим остановлен"
                                color: root.cText; font.bold: true
                            }
                            Label {
                                text: root.mon.finishReason
                                color: root.cSub
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Прогресс дерева:"; color: root.cSub }
                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: Math.max(1, root.mon.progressMax)
                        value: root.mon.progress
                    }
                    Label {
                        text: root.mon.progress + " / " + root.mon.progressMax
                        color: root.cSub; font.family: "Consolas"
                    }
                }
                Label { text: "Клапаны:"; color: root.cSub }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    Repeater {
                        model: root.valveOrder
                        delegate: Rectangle {
                            required property string modelData
                            readonly property bool open: root.mon.valveStates[modelData] === true
                            width: chipRow.width + 16
                            height: 26
                            radius: 4
                            color: open ? root.stateColor(2) : root.cField
                            border.color: root.cBorder; border.width: 1
                            Row {
                                id: chipRow
                                anchors.centerIn: parent
                                spacing: 6
                                Label { text: modelData; color: root.cText; font.bold: true; font.pointSize: 9 }
                                Label { text: open ? "OPEN" : "closed"; color: open ? root.cText : root.cSub; font.pointSize: 8 }
                            }
                        }
                    }
                }
            }
        }
    }
}
