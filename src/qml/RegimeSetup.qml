import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.regimeTaskTreeSingleton 1.0
import com.grams.prototable

// ─────────────────────────────────────────────────────────────────────────────
//  RegimeSetup — настройка ОДНОГО режима из очереди RunTable.
//
//  Открывается по нажатию на кнопку режима в RunTable (SideMenu.openRegimePage)
//  и настраивает именно ту строку, по которой нажали: regimeIndex — её номер.
//
//  Разделение с вкладкой «Режим» (RegimeApiSandbox) намеренное и проходит по
//  вопросу, на который отвечает страница:
//    здесь            — «что запустить»: параметры прогона, доступные оператору;
//    вкладка «Режим»  — «что сейчас происходит»: развёртка рецепта, клапаны,
//                       показания датчиков, причина завершения. Это отладочный
//                       просмотр, и настроек в нём быть не должно.
//
//  ПАРАМЕТРЫ ПРИМЕНЯЮТСЯ СРАЗУ, а не по кнопке «Старт»: старт живёт теперь на
//  RunTable и про эту страницу не знает. Каждый контрол пушит значение в
//  RegimeTaskTree в своём обработчике — иначе набранное здесь просто не дошло
//  бы до прогона.
// ─────────────────────────────────────────────────────────────────────────────
Item {
    id: root

    // Какую строку очереди настраиваем. -1 — страницу ещё не открывали.
    property string regimeName:  ""
    property int    regimeIndex: -1

    // ── Палитра (та же, что на вкладке «Режим») ───────────────────────────────
    readonly property color cBg:     "#464646"
    readonly property color cCard:   "#3A3A3A"
    readonly property color cBorder: "#ABDBDD"
    readonly property color cText:   "#FFFFFF"
    readonly property color cSub:    "#9090A0"
    readonly property color cField:  "#2A2A2A"

    readonly property var mon: RegimeTaskTree.vacuumMonitor
    readonly property bool isVacuum: regimeName === "Вакуум"

    // Строка очереди: время и повторы задаются ТАМ и читаются здесь.
    // Дублировать их редактирование на двух экранах нельзя — разошлись бы.
    property var regimeRow: null

    function reloadRow() {
        regimeRow = (regimeIndex >= 0 && RegimeManager.model
                     && regimeIndex < RegimeManager.model.getRowCount())
                    ? RegimeManager.model.getRegime(regimeIndex)
                    : null
    }
    onRegimeIndexChanged: reloadRow()
    Connections {
        target: RegimeManager
        function onRegimeDataUpdated() { root.reloadRow() }
    }

    // ── Значения, которые страница пушит в C++ ────────────────────────────────
    //
    // Флаги блока C хранятся в терминах «откачивать», а не «пропустить»:
    // оператор включает объёмы в откачку, а выключением — исключает. В C++ они
    // уходят инвертированными, потому что setVacuumOptions принимает skip*.
    property bool pumpC1: true      // RK300 / S3 (К135)
    property bool pumpC2: true      // RK50  / S2 (К133)
    property bool pumpC3: true      // RK10  / S1 (К131)
    property bool secondTract: false
    property bool pumpCheck:   true
    property bool continuousPumping: false

    property real targetVacPa:       40.0
    property int  holdSec:           60
    property int  foreVacTimeoutSec: 300

    property real finalTargetPa: 0.0133
    property int  evacTimeSec:   1800

    // setVacuumOptions принимает все четыре флага разом, поэтому любое
    // изменение отправляет полный набор.
    function pushOptions() {
        RegimeTaskTree.setVacuumOptions(!pumpC3, !pumpC2, !pumpC1, secondTract)
    }
    function pushForevac() {
        RegimeTaskTree.setVacuumForevacTarget(targetVacPa, holdSec, foreVacTimeoutSec)
    }
    function pushFinal() {
        RegimeTaskTree.setVacuumFinalTarget(finalTargetPa, evacTimeSec)
    }

    function fmtHms(seconds) {
        if (seconds === undefined || seconds === null || seconds < 0)
            return "--:--:--"
        var h = Math.floor(seconds / 3600)
        var m = Math.floor((seconds % 3600) / 60)
        var s = Math.floor(seconds % 60)
        return String(h).padStart(2, "0") + ":"
             + String(m).padStart(2, "0") + ":"
             + String(s).padStart(2, "0")
    }
    function fmtPa(v) {
        if (!isFinite(v)) return "—"
        if (v >= 1000 || (v > 0 && v < 0.01)) return v.toExponential(2) + " Па"
        return v.toFixed(2) + " Па"
    }
    function stateColor(s) {
        switch (s) {
        case 1:  return "#E6C33A"   // Running
        case 2:  return "#4CAF50"   // Success
        case 3:  return "#C0392B"   // Error
        case 4:  return "#3AA6B9"   // Cancelled
        case 5:  return "#5A5A5A"   // Skipped
        default: return "#808080"   // Initial
        }
    }

    Rectangle { anchors.fill: parent; color: root.cBg }

    component Card : Rectangle {
        default property alias cardData: inner.data
        Layout.fillWidth: true
        color: root.cCard
        radius: 6
        border.color: root.cBorder
        border.width: 1
        implicitHeight: inner.implicitHeight + 12
        ColumnLayout {
            id: inner
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 6 }
            spacing: 4
        }
    }
    component Cap : Label {
        color: root.cSub
        font.pointSize: 9
    }
    component Chk : CheckBox {
        padding: 2
        font.pointSize: 9
        ToolTip.visible: hovered && ToolTip.text.length > 0
        ToolTip.delay: 500
    }
    component Num : SpinBox {
        font.pointSize: 9
        padding: 2
        Layout.preferredWidth: 100
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.margins: 6
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 6

            // ── Шапка: какая строка очереди настраивается ─────────────────────
            Card {
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: root.regimeName.length ? root.regimeName : "Режим не выбран"
                        color: root.cText
                        font.pointSize: 12
                        font.bold: true
                    }
                    Cap { text: root.regimeIndex >= 0 ? "строка " + (root.regimeIndex + 1) : "" }
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

                // Время прогона и повторы — из строки RunTable, и правятся ТАМ.
                // Здесь они показаны, чтобы настройки читались вместе с
                // потолком времени, в который они обязаны уложиться.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Cap { text: "Общее время прогона:" }
                    Label {
                        text: root.regimeRow ? root.fmtHms(root.regimeRow.maxTime) : "--:--:--"
                        color: root.cText
                        font.family: "Consolas"
                        font.bold: true
                    }
                    Cap { text: "повторов:" }
                    Label {
                        text: root.regimeRow ? root.regimeRow.repeatCount : "—"
                        color: root.cText
                        font.family: "Consolas"
                    }
                    Item { Layout.fillWidth: true }
                    Cap {
                        text: "задаются в RunTable"
                        font.pointSize: 8
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: root.isVacuum
                    wrapMode: Text.WordWrap
                    font.pointSize: 8
                    color: root.cSub
                    text: "Время строки — жёсткий потолок всего прогона: любое ожидание "
                        + "режима обрезается остатком, а исчерпание завершает прогон "
                        + "штатно и с названной причиной."
                }
            }

            // ── Настройки «Вакуума» ──────────────────────────────────────────
            Card {
                visible: root.isVacuum

                Label { text: "Что откачиваем"; color: root.cBorder; font.bold: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    // Галка = «откачивать». Выключение исключает объём из
                    // откачки; по умолчанию откачивается весь блок C.
                    Cap { text: "Откачка C:" }
                    Chk {
                        text: "C1"; ToolTip.text: "RK300 / S3 (К135). Снять — не откачивать C1"
                        checked: root.pumpC1
                        onToggled: { root.pumpC1 = checked; root.pushOptions() }
                    }
                    Chk {
                        text: "C2"; ToolTip.text: "RK50 / S2 (К133). Снять — не откачивать C2"
                        checked: root.pumpC2
                        onToggled: { root.pumpC2 = checked; root.pushOptions() }
                    }
                    Chk {
                        text: "C3"; ToolTip.text: "RK10 / S1 (К131). Снять — не откачивать C3"
                        checked: root.pumpC3
                        onToggled: { root.pumpC3 = checked; root.pushOptions() }
                    }
                    Rectangle {
                        implicitWidth: 1; Layout.preferredHeight: 18
                        color: root.cBorder; opacity: 0.4
                    }
                    Chk {
                        // Ф3 под вопросом целиком: фаза открывает К192 (SL1,
                        // выход второго тракта в атмосферу) и следом К179
                        // (SL2, турбонасос). Требует подтверждения схемой —
                        // до него выключена по умолчанию.
                        text: "2-й тракт ⚠"
                        ToolTip.text: "Ф3: К192 (выход в атмосферу) → К179 (турбонасос), "
                                    + "s20–s24. ТРЕБУЕТ ПОДТВЕРЖДЕНИЯ СХЕМОЙ"
                        checked: root.secondTract
                        onToggled: { root.secondTract = checked; root.pushOptions() }
                    }
                    Chk {
                        text: "dP/dt"
                        ToolTip.text: "Проверка скорости откачки после открытия К176"
                        checked: root.pumpCheck
                        onToggled: {
                            root.pumpCheck = checked
                            RegimeTaskTree.setVacuumPumpCheck(checked)
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Chk {
                        text: "непрерывная откачка после прогона"
                        ToolTip.text: "Оставить тракт (камера E/F, эталон B, бочка) открытым "
                                    + "на турбонасосе; форвакуумный К176 закрыт. "
                                    + "Меняется и на ходу"
                        checked: root.continuousPumping
                        onToggled: {
                            root.continuousPumping = checked
                            // Хвост рецепта выполняется в самом конце прогона,
                            // поэтому решение, принятое уже во время режима,
                            // обязано до него дойти.
                            RegimeTaskTree.setVacuumContinuousPumping(checked)
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            Card {
                visible: root.isVacuum
                Label { text: "Форвакуум 11.5–11.7"; color: root.cBorder; font.bold: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Cap { text: "до:" }
                    // Текстом, а не SpinBox: диапазон 1e-4…1e5 Па — пять порядков.
                    TextField {
                        id: targetField
                        Layout.preferredWidth: 78
                        text: root.targetVacPa
                        font.pointSize: 9
                        font.family: "Consolas"
                        horizontalAlignment: TextInput.AlignRight
                        validator: DoubleValidator {
                            bottom: 0.0001; top: 100000
                            notation: DoubleValidator.ScientificNotation
                        }
                        onEditingFinished: {
                            var v = parseFloat(text)
                            if (!isNaN(v) && v > 0) { root.targetVacPa = v; root.pushForevac() }
                            else text = root.targetVacPa
                        }
                    }
                    Cap { text: "Па (ДВ301), удержать, с" }
                    Num {
                        from: 1; to: 3600; stepSize: 5
                        value: root.holdSec
                        onValueModified: { root.holdSec = value; root.pushForevac() }
                    }
                    Cap { text: "таймаут, с" }
                    Num {
                        from: 10; to: 7200; stepSize: 30
                        value: root.foreVacTimeoutSec
                        onValueModified: { root.foreVacTimeoutSec = value; root.pushForevac() }
                    }
                    Button {
                        text: "⟲"
                        flat: true
                        font.pointSize: 9
                        ToolTip.text: "По умолчанию: 40 Па / 60 с / 300 с"
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        onClicked: {
                            root.targetVacPa = 40.0
                            root.holdSec = 60
                            root.foreVacTimeoutSec = 300
                            targetField.text = root.targetVacPa
                            root.pushForevac()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            Card {
                visible: root.isVacuum
                Label { text: "Финальная откачка камеры 11.10"; color: root.cBorder; font.bold: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Cap { text: "цель:" }
                    TextField {
                        id: finalField
                        Layout.preferredWidth: 78
                        text: root.finalTargetPa
                        font.pointSize: 9
                        font.family: "Consolas"
                        horizontalAlignment: TextInput.AlignRight
                        validator: DoubleValidator {
                            bottom: 0.000001; top: 100000
                            notation: DoubleValidator.ScientificNotation
                        }
                        onEditingFinished: {
                            var v = parseFloat(text)
                            if (!isNaN(v) && v > 0) { root.finalTargetPa = v; root.pushFinal() }
                            else text = root.finalTargetPa
                        }
                    }
                    Cap { text: "Па, время (EvacTime), с" }
                    Num {
                        from: 1; to: 86399; stepSize: 60
                        value: root.evacTimeSec
                        onValueModified: { root.evacTimeSec = value; root.pushFinal() }
                    }
                    Item { Layout.fillWidth: true }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pointSize: 8
                    color: root.cSub
                    // Правило выбора насоса — не настройка, а следствие хода
                    // прогона, и оператор должен его знать заранее.
                    text: "Насос выбирается по факту: если переход на турбомолекулярный "
                        + "(12.2) состоялся, 11.10 качает К179 и форвакуумный К176 больше "
                        + "не открывает. Форвакуум остаётся только там, где перехода не "
                        + "было или был откат — тогда контроль идёт по ДВ301."
                }
            }

            // ── Прогресс шагов режима ────────────────────────────────────────
            //
            // Полная развёртка со всеми подробностями живёт на вкладке «Режим».
            // Здесь — компактный список: где сейчас режим и что уже пройдено.
            Card {
                visible: root.isVacuum
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Прогресс шагов"; color: root.cBorder; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Cap {
                        text: root.mon.currentLabel.length ? root.mon.currentLabel : "—"
                        elide: Text.ElideLeft
                        Layout.maximumWidth: 260
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: root.mon.budgetLimited
                    Cap { text: "время прогона:" }
                    Label {
                        text: root.fmtHms(root.mon.budgetElapsedSec) + " / "
                              + root.fmtHms(root.mon.budgetTotalSec)
                        color: root.cText
                        font.family: "Consolas"
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: Math.max(1, root.mon.budgetTotalSec)
                        value: root.mon.budgetElapsedSec
                    }
                    Label {
                        text: "осталось " + root.fmtHms(root.mon.budgetRemainingSec)
                        color: root.mon.budgetRemainingSec === 0 ? root.stateColor(3) : root.cSub
                        font.pointSize: 8
                        font.family: "Consolas"
                    }
                }

                Repeater {
                    model: root.mon.steps
                    delegate: Rectangle {
                        required property var model
                        Layout.fillWidth: true
                        implicitHeight: 22
                        radius: 3
                        color: model.state === 1 ? root.cField : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6 + model.depth * 18
                            anchors.rightMargin: 6
                            spacing: 8
                            Rectangle {
                                implicitWidth: 10; implicitHeight: 10; radius: 2
                                color: root.stateColor(model.state)
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
                                font.pointSize: 9
                                font.bold: model.depth === 0
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                text: model.sRange
                                color: root.cSub
                                font.pointSize: 8
                                font.family: "Consolas"
                            }
                        }
                    }
                }

                // Пропуски и предупреждения — рядом с шагами, а не в логе:
                // «этап не выполнялся» обязано быть видно там же, где шаги.
                Repeater {
                    model: root.mon.skippedStages
                    delegate: Label {
                        required property string modelData
                        Layout.fillWidth: true
                        text: "⊘ " + modelData
                        color: root.stateColor(5)
                        font.pointSize: 8
                        wrapMode: Text.WordWrap
                    }
                }
                Repeater {
                    model: root.mon.warnings
                    delegate: Label {
                        required property string modelData
                        Layout.fillWidth: true
                        text: "⚠ " + modelData
                        color: "#d0a050"
                        font.pointSize: 8
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // ── Режимы без настроек ──────────────────────────────────────────
            Card {
                visible: !root.isVacuum && root.regimeIndex >= 0
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: root.cSub
                    text: "У режима «" + root.regimeName + "» настраиваемых параметров "
                        + "пока нет. Время прогона и число повторов задаются в строке "
                        + "RunTable; ход выполнения виден на вкладке «Режим»."
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
