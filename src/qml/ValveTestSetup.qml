import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Grams.regimeTaskTreeSingleton 1.0

Item {
    id: root

    // ── Palette ───────────────────────────────────────────────────────────────
    readonly property color cBg:       "#464646"
    readonly property color cCard:     "#3A3A3A"
    readonly property color cBorder:   "#ABDBDD"
    readonly property color cAccent:   "#594E74"
    readonly property color cRunning:  "#4CAF50"
    readonly property color cText:     "#FFFFFF"
    readonly property color cSub:      "#9090A0"
    readonly property color cValveBg:  "#2A2A2A"
    readonly property color cDanger:   "#8B3030"
    readonly property color cMerge:    "#2D5A7A"

    // ── Global params (stored here, pushed to C++ on Apply) ───────────────────
    property int globalRepeats:     1
    property int globalPauseBefore: 0
    property int globalPauseAfter:  0

    // ── Step model ────────────────────────────────────────────────────────────
    // One row = one step.  valves is comma-joined valve names.
    ListModel { id: stepsModel }

    Component.onCompleted: {
        let av = RegimeTaskTree.availableValves
        for (let i = 0; i < av.length; ++i)
            stepsModel.append({ valves: av[i], pauseBefore: 0, dwell: 2, pauseAfter: 0 })
    }

    // ── JS helpers ────────────────────────────────────────────────────────────

    function valveList(idx) {
        if (idx < 0 || idx >= stepsModel.count) return []
        return stepsModel.get(idx).valves.split(",").filter(v => v.length > 0)
    }

    function setValves(idx, list) {
        stepsModel.setProperty(idx, "valves", list.join(","))
    }

    function removeValveFromStep(stepIdx, name) {
        let list = valveList(stepIdx).filter(v => v !== name)
        if (list.length === 0) stepsModel.remove(stepIdx)
        else                   setValves(stepIdx, list)
    }

    function addValveToStep(stepIdx, name) {
        let list = valveList(stepIdx)
        if (name.length === 0 || list.includes(name)) return
        list.push(name)
        setValves(stepIdx, list)
    }

    function moveUp(idx) {
        if (idx > 0) stepsModel.move(idx, idx - 1, 1)
    }

    function moveDown(idx) {
        if (idx < stepsModel.count - 1) stepsModel.move(idx, idx + 1, 1)
    }

    function mergeWithPrev(idx) {
        if (idx <= 0) return
        let merged = stepsModel.get(idx - 1).valves + "," + stepsModel.get(idx).valves
        stepsModel.setProperty(idx - 1, "valves", merged)
        stepsModel.remove(idx)
    }

    function mergeWithNext(idx) {
        if (idx >= stepsModel.count - 1) return
        let merged = stepsModel.get(idx).valves + "," + stepsModel.get(idx + 1).valves
        stepsModel.setProperty(idx, "valves", merged)
        stepsModel.remove(idx + 1)
    }

    function applyConfig() {
        let arr = []
        for (let i = 0; i < stepsModel.count; ++i) {
            let s = stepsModel.get(i)
            arr.push({
                valves:      s.valves.split(",").filter(v => v.length > 0),
                pauseBefore: s.pauseBefore,
                dwell:       s.dwell,
                pauseAfter:  s.pauseAfter
            })
        }
        RegimeTaskTree.setValveTestSteps(arr, globalRepeats, globalPauseBefore, globalPauseAfter)
    }

    // ── Root layout ───────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // ── Global params block ───────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 72
            color: cCard
            border.color: cAccent
            border.width: 1
            radius: 4

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Text {
                    text: qsTr("Глобальные параметры")
                    color: cText
                    font { family: "Verdana"; pointSize: 9; bold: true }
                }

                RowLayout {
                    spacing: 10

                    Text { text: qsTr("Повторений:"); color: cSub; font { family: "Verdana"; pointSize: 8 } }
                    SpinBox {
                        from: 1; to: 100; value: globalRepeats
                        implicitWidth: 70; implicitHeight: 26
                        onValueChanged: globalRepeats = value
                    }

                    Item { Layout.preferredWidth: 8 }

                    Text { text: qsTr("Пауза до (с):"); color: cSub; font { family: "Verdana"; pointSize: 8 } }
                    SpinBox {
                        from: 0; to: 600; value: globalPauseBefore
                        implicitWidth: 70; implicitHeight: 26
                        onValueChanged: globalPauseBefore = value
                    }

                    Item { Layout.preferredWidth: 8 }

                    Text { text: qsTr("Пауза после (с):"); color: cSub; font { family: "Verdana"; pointSize: 8 } }
                    SpinBox {
                        from: 0; to: 600; value: globalPauseAfter
                        implicitWidth: 70; implicitHeight: 26
                        onValueChanged: globalPauseAfter = value
                    }
                }
            }
        }

        // ── Step list ─────────────────────────────────────────────────────────
        ScrollView {
            id: stepsScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                width: stepsScroll.width
                spacing: 0

                // ── Steps ─────────────────────────────────────────────────────
                Repeater {
                    model: stepsModel

                    delegate: Column {
                        id: stepDelegate

                        required property int    index
                        required property string valves
                        required property int    pauseBefore
                        required property int    dwell
                        required property int    pauseAfter

                        width: parent.width
                        spacing: 0

                        // ── Step card ─────────────────────────────────────────
                        Rectangle {
                            width: parent.width
                            height: stepRow.implicitHeight + 16
                            color: cCard
                            radius: 4
                            border.color: RegimeTaskTree.running ? cRunning : cBorder
                            border.width: 1

                            RowLayout {
                                id: stepRow
                                anchors {
                                    left: parent.left; right: parent.right
                                    verticalCenter: parent.verticalCenter
                                    margins: 8
                                }
                                spacing: 8

                                // ── Valve squares ─────────────────────────────
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 5

                                    Repeater {
                                        // The 'valves' required property is reactive:
                                        // re-evaluates whenever the model role changes.
                                        model: valves.split(",").filter(v => v.length > 0)

                                        delegate: Rectangle {
                                            id: valveSquare
                                            required property string modelData   // valve name
                                            required property int    index

                                            width: 68; height: 68
                                            color: cValveBg
                                            border.color: cBorder
                                            border.width: 1
                                            radius: 4

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 4
                                                spacing: 2

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: modelData
                                                    color: cText
                                                    font { family: "Verdana"; pointSize: 7; bold: true }
                                                    horizontalAlignment: Text.AlignHCenter
                                                    wrapMode: Text.Wrap
                                                }

                                                Button {
                                                    Layout.alignment: Qt.AlignHCenter
                                                    text: "×"
                                                    implicitWidth: 22; implicitHeight: 18
                                                    font { pointSize: 9; bold: true }
                                                    palette { button: cDanger }
                                                    onClicked: removeValveFromStep(stepDelegate.index, modelData)
                                                }
                                            }
                                        }
                                    }

                                    // ── Add-valve row ─────────────────────────
                                    RowLayout {
                                        height: 68
                                        spacing: 4

                                        ComboBox {
                                            id: valvePicker
                                            Layout.preferredWidth: 88
                                            Layout.preferredHeight: 26
                                            model: RegimeTaskTree.availableValves
                                            font.pointSize: 7
                                        }

                                        Button {
                                            text: "+"
                                            Layout.preferredWidth: 26; Layout.preferredHeight: 26
                                            font { pointSize: 10; bold: true }
                                            onClicked: addValveToStep(stepDelegate.index, valvePicker.currentText)
                                        }
                                    }
                                }

                                // ── Timing spinboxes ──────────────────────────
                                GridLayout {
                                    columns: 2
                                    rowSpacing: 4
                                    columnSpacing: 4

                                    Text { text: qsTr("До (с):"); color: cSub; font { family: "Verdana"; pointSize: 7 } }
                                    SpinBox {
                                        from: 0; to: 600
                                        value: pauseBefore
                                        Layout.preferredWidth: 66; Layout.preferredHeight: 24
                                        font.pointSize: 7
                                        onValueChanged: {
                                            if (stepDelegate.index < stepsModel.count)
                                                stepsModel.setProperty(stepDelegate.index, "pauseBefore", value)
                                        }
                                    }

                                    Text { text: qsTr("Держать (с):"); color: cSub; font { family: "Verdana"; pointSize: 7 } }
                                    SpinBox {
                                        from: 1; to: 600
                                        value: dwell
                                        Layout.preferredWidth: 66; Layout.preferredHeight: 24
                                        font.pointSize: 7
                                        onValueChanged: {
                                            if (stepDelegate.index < stepsModel.count)
                                                stepsModel.setProperty(stepDelegate.index, "dwell", value)
                                        }
                                    }

                                    Text { text: qsTr("После (с):"); color: cSub; font { family: "Verdana"; pointSize: 7 } }
                                    SpinBox {
                                        from: 0; to: 600
                                        value: pauseAfter
                                        Layout.preferredWidth: 66; Layout.preferredHeight: 24
                                        font.pointSize: 7
                                        onValueChanged: {
                                            if (stepDelegate.index < stepsModel.count)
                                                stepsModel.setProperty(stepDelegate.index, "pauseAfter", value)
                                        }
                                    }
                                }

                                // ── Step control column ───────────────────────
                                Column {
                                    spacing: 3

                                    Button {
                                        text: "↑"; implicitWidth: 30; implicitHeight: 22
                                        font.pointSize: 9; enabled: stepDelegate.index > 0
                                        onClicked: moveUp(stepDelegate.index)
                                    }
                                    Button {
                                        text: "↓"; implicitWidth: 30; implicitHeight: 22
                                        font.pointSize: 9
                                        enabled: stepDelegate.index < stepsModel.count - 1
                                        onClicked: moveDown(stepDelegate.index)
                                    }
                                    Button {
                                        text: "∥↑"; implicitWidth: 30; implicitHeight: 22
                                        font.pointSize: 7; enabled: stepDelegate.index > 0
                                        palette { button: cMerge }
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Объединить с предыдущим (параллельно)")
                                        onClicked: mergeWithPrev(stepDelegate.index)
                                    }
                                    Button {
                                        text: "∥↓"; implicitWidth: 30; implicitHeight: 22
                                        font.pointSize: 7
                                        enabled: stepDelegate.index < stepsModel.count - 1
                                        palette { button: cMerge }
                                        ToolTip.visible: hovered
                                        ToolTip.text: qsTr("Объединить со следующим (параллельно)")
                                        onClicked: mergeWithNext(stepDelegate.index)
                                    }
                                    Button {
                                        text: "×"; implicitWidth: 30; implicitHeight: 22
                                        font.pointSize: 9; palette { button: cDanger }
                                        onClicked: stepsModel.remove(stepDelegate.index)
                                    }
                                }
                            }
                        }

                        // ── Arrow separator ───────────────────────────────────
                        Item {
                            width: parent.width
                            height: stepDelegate.index < stepsModel.count - 1 ? 18 : 0
                            visible: stepDelegate.index < stepsModel.count - 1

                            Text {
                                anchors.centerIn: parent
                                text: "↓"
                                color: cSub
                                font { pointSize: 11; bold: true }
                            }
                        }
                    }
                }

                // ── Add step button ───────────────────────────────────────────
                Button {
                    width: parent.width
                    height: 30
                    text: qsTr("+ Добавить шаг")
                    font { family: "Verdana"; pointSize: 8 }
                    onClicked: {
                        let av = RegimeTaskTree.availableValves
                        stepsModel.append({
                            valves:      av.length > 0 ? av[0] : "",
                            pauseBefore: 0,
                            dwell:       2,
                            pauseAfter:  0
                        })
                    }
                }
            }
        }

        // ── Bottom control bar ────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 48
            color: cCard
            border.color: cAccent
            border.width: 1
            radius: 4

            RowLayout {
                anchors.centerIn: parent
                spacing: 8

                Button {
                    text: qsTr("Применить")
                    font { family: "Verdana"; pointSize: 9 }
                    Layout.preferredHeight: 32
                    onClicked: applyConfig()
                }

                Button {
                    text: qsTr("Старт")
                    font { family: "Verdana"; pointSize: 9; bold: true }
                    Layout.preferredHeight: 32
                    enabled: !RegimeTaskTree.running
                    palette { button: "#2D6A2D" }
                    onClicked: { applyConfig(); RegimeTaskTree.startAll() }
                }

                Button {
                    text: RegimeTaskTree.paused ? qsTr("Продолжить") : qsTr("Пауза")
                    font { family: "Verdana"; pointSize: 9 }
                    Layout.preferredHeight: 32
                    enabled: RegimeTaskTree.running
                    onClicked: RegimeTaskTree.paused ? RegimeTaskTree.resume() : RegimeTaskTree.pause()
                }

                Button {
                    text: qsTr("Стоп")
                    font { family: "Verdana"; pointSize: 9; bold: true }
                    Layout.preferredHeight: 32
                    enabled: RegimeTaskTree.running
                    palette { button: cDanger }
                    onClicked: RegimeTaskTree.stop()
                }
            }
        }
    }
}
