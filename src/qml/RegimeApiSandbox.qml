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

        contentItem: ColumnLayout {
            spacing: 16
            Label {
                text: operatorDialog.message
                color: root.cText
                wrapMode: Text.WordWrap
                Layout.preferredWidth: 360
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
    property int  stepPauseSec: 1   // общий дефолт settle-паузы
    property int  reliefHoldSec: 1  // удержание К118 при сбросе (≥1 с)

    // Индивидуальные задержки на каждый шаг (ключ шага → секунды).
    property var stepDelays: ({})
    Component.onCompleted: {
        var keys = RegimeTaskTree.vacuumStepKeys()
        var d = {}
        for (var i = 0; i < keys.length; ++i)
            d[keys[i].key] = root.stepPauseSec
        root.stepDelays = d
    }

    component Card : Rectangle {
        default property alias cardData: inner.data
        Layout.fillWidth: true
        color: root.cCard
        radius: 8
        border.color: root.cBorder
        border.width: 1
        implicitHeight: inner.implicitHeight + 24
        ColumnLayout {
            id: inner
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            spacing: 8
        }
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.margins: 12
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 14

            // ── Заголовок + селектор режима ────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Развёртка рецепта режима"
                    color: root.cText
                    font.pointSize: 16
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Label { text: "Режим:"; color: root.cSub }
                ComboBox {
                    id: regimeSelector
                    model: ["Вакуум"]     // задел под будущие режимы
                    Layout.preferredWidth: 180
                }
            }

            // ── Опции + управление ─────────────────────────────────────────────
            Card {
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    Label { text: "Пропуск объёмов C:"; color: root.cSub }
                    // Метки в терминах интерфейса C1/C2/C3 (ТЗ REQ-005); в скобках — легаси RK.
                    CheckBox { text: "C1 (RK300)"; checked: root.skipRK300; onToggled: root.skipRK300 = checked }
                    CheckBox { text: "C3 (RK10)";  checked: root.skipRK10;  onToggled: root.skipRK10  = checked }
                    CheckBox { text: "C2 (RK50)";  checked: root.skipRK50;  onToggled: root.skipRK50  = checked }
                    CheckBox { text: "2-й тракт"; checked: root.secondTract; onToggled: root.secondTract = checked }
                    CheckBox { text: "форвакуум"; checked: root.foreVacuum; onToggled: root.foreVacuum = checked }
                    CheckBox { text: "проверка dP/dt"; checked: root.pumpCheck; onToggled: root.pumpCheck = checked }
                    Item { Layout.fillWidth: true }
                    Label { text: "Сброс К118, с:"; color: root.cSub }
                    SpinBox { from: 1; to: 30; value: root.reliefHoldSec; onValueModified: root.reliefHoldSec = value }
                    Label { text: "Повторы:"; color: root.cSub }
                    SpinBox { from: 1; to: 99; value: root.repeats; onValueModified: root.repeats = value }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Button {
                        text: "▶ Старт"
                        enabled: !RegimeTaskTree.running
                        onClicked: {
                            RegimeTaskTree.setVacuumOptions(root.skipRK10, root.skipRK50,
                                                            root.skipRK300, root.secondTract)
                            RegimeTaskTree.setVacuumForevac(root.foreVacuum)
                            RegimeTaskTree.setVacuumPumpCheck(root.pumpCheck)
                            RegimeTaskTree.setVacuumStepPauseMs(root.stepPauseSec * 1000)
                            RegimeTaskTree.setVacuumStepDelays(root.stepDelays)
                            RegimeTaskTree.setVacuumReliefHoldSec(root.reliefHoldSec)
                            RegimeTaskTree.startAll()
                        }
                    }
                    Button {
                        text: RegimeTaskTree.paused ? "▷ Резюм" : "⏸ Пауза"
                        enabled: RegimeTaskTree.running
                        onClicked: RegimeTaskTree.paused ? RegimeTaskTree.resume()
                                                         : RegimeTaskTree.pause()
                    }
                    Button {
                        text: "⏹ Стоп"
                        enabled: RegimeTaskTree.running
                        onClicked: RegimeTaskTree.stop()
                    }
                    Button {
                        text: "⟲ Сброс"
                        enabled: !RegimeTaskTree.running
                        onClicked: root.mon.reset()
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: RegimeTaskTree.running
                              ? (RegimeTaskTree.paused ? "на паузе" : "выполняется")
                              : "остановлен"
                        color: RegimeTaskTree.running ? root.stateColor(1) : root.cSub
                        font.bold: true
                    }
                }
            }

            // ── Задержки шагов (индивидуально «для КАЖДОГО действия») ──────────
            Card {
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Задержка после каждого шага, с"; color: root.cBorder; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label { text: "общий дефолт:"; color: root.cSub }
                    SpinBox {
                        from: 0; to: 30; value: root.stepPauseSec
                        onValueModified: {
                            root.stepPauseSec = value
                            var d = {}
                            var keys = RegimeTaskTree.vacuumStepKeys()
                            for (var i = 0; i < keys.length; ++i)
                                d[keys[i].key] = value
                            root.stepDelays = d
                        }
                    }
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 12
                    Repeater {
                        model: RegimeTaskTree.vacuumStepKeys()
                        delegate: RowLayout {
                            required property var modelData
                            spacing: 6
                            Label { text: modelData.label; color: root.cText; font.pointSize: 9 }
                            SpinBox {
                                from: 0; to: 60
                                value: root.stepDelays[modelData.key] !== undefined ? root.stepDelays[modelData.key] : root.stepPauseSec
                                onValueModified: {
                                    var d = root.stepDelays
                                    d[modelData.key] = value
                                    root.stepDelays = d
                                }
                            }
                        }
                    }
                }
            }

            // ── Развёртка рецепта (дерево узлов + состояния) ───────────────────
            Card {
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Рецепт «Вакуум» — узлы и состояние"; color: root.cBorder; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: "текущая фаза: " + (root.mon.currentLabel.length ? root.mon.currentLabel : "—")
                        color: root.cSub
                    }
                }
                Repeater {
                    model: root.mon.steps
                    delegate: Rectangle {
                        required property var model
                        Layout.fillWidth: true
                        implicitHeight: 30
                        color: model.depth > 0 ? root.cField : "transparent"
                        radius: 4
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8 + model.depth * 26
                            anchors.rightMargin: 8
                            spacing: 10
                            // Чип состояния
                            Rectangle {
                                width: 14; height: 14; radius: 3
                                color: root.stateColor(model.state)
                                border.color: root.cBg; border.width: 1
                                // «Спиннер»: пульсация при Running
                                SequentialAnimation on opacity {
                                    running: model.state === 1
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 0.35; duration: 500 }
                                    NumberAnimation { to: 1.0;  duration: 500 }
                                }
                            }
                            Label {
                                text: (model.phase.length ? model.phase + "  " : "") + model.label
                                color: root.cText
                                font.bold: model.depth === 0
                            }
                            Label { text: model.sRange; color: root.cSub; font.pointSize: 8; font.family: "Consolas" }
                            Item { Layout.fillWidth: true }
                            Label {
                                text: root.stateText(model.state)
                                color: root.stateColor(model.state)
                                font.pointSize: 9
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
