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

    // ── Операторский диалог (OperatorBus) ─────────────────────────────────────
    //
    // ТЗ требует решения оператора не в одной точке, а минимум в четырёх, и
    // они не равнозначны: при over range ДВ302 «продолжить как есть» означало
    // бы открыть К179 на непрощавшемся тракте. Поэтому заголовок выбирается
    // по коду события, а не один на все случаи.
    Connections {
        target: RegimeTaskTree.operatorBus
        function onDecisionRequired(code, message) {
            operatorDialog.code = code
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
        property int    code: 0
        // Коды из OperatorBus::Event.
        readonly property var titles: ({
            1: "Проверка откачки",
            2: "ДВ301 вне диапазона",
            3: "ДВ302 вне диапазона",
            4: "Клапан К176 не подтверждён"
        })
        anchors.centerIn: Overlay.overlay
        modal: true
        closePolicy: Popup.NoAutoClose
        title: titles[code] !== undefined ? titles[code] : "Требуется решение оператора"
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

    // ── Параметры цепочки «Напуск → Натекание» ───────────────────────────────
    // Это параметры ПРОГОНА (меняются от прогона к прогону и задают результат),
    // поэтому им место в интерфейсе. Пороги гейтов цепочки сюда не выносятся —
    // они из профиля, как и пороги безопасности вакуума.
    property string supplyPort:        "AR1"
    property int    supplyOpenTimeSec: 10
    property real   supplyLimitBar:    10.0
    property string leakageValve:      "R1"
    property int    leakageDurationSec: 60
    property real   leakageTargetDeltaBar: 0.0

    // Параметры цепочки принимаются набором, поэтому любое изменение
    // отправляет весь набор целиком. Раньше это делала кнопка «Старт»;
    // теперь старт на RunTable и про эту вкладку не знает.
    function pushSupply() {
        RegimeTaskTree.setSupplyParams(supplyPort, supplyOpenTimeSec * 1000, supplyLimitBar)
    }
    function pushLeakage() {
        RegimeTaskTree.setLeakageParams(leakageValve, leakageDurationSec, leakageTargetDeltaBar)
    }

    // Длительности зафиксированы в C++ (VacuumTreeContext) и из UI не правятся:
    // шаг 3 с, сброс К118 10 с, «мёртвая зона» dP/dt 30 с.
    readonly property string fixedTimings: "шаг 3 с · сброс К118 10 с · dP/dt через 30 с"

    // мм:сс для длительностей этапов (турбо-откачка идёт минутами).
    function fmtSec(v) {
        var m = Math.floor(v / 60)
        var sec = Math.floor(v % 60)
        return m + ":" + (sec < 10 ? "0" : "") + sec
    }

    // «Должно быть» для строки 11.x — из монитора, чтобы UI показывал реально
    // применённое значение (после qBound в C++), а не то, что набрано в поле.
    function fmtPa(v) {
        if (!isFinite(v)) return "—"
        if (v >= 1000 || (v > 0 && v < 0.01)) return v.toExponential(2) + " Па"
        return v.toFixed(2) + " Па"
    }

    // Формат времени как в столбце RunTable (ЧЧ:ММ:СС): одно число должно
    // выглядеть одинаково в таблице и здесь, иначе их не сопоставить глазом.
    function formatHms(seconds) {
        if (seconds === undefined || seconds < 0)
            return "--:--:--"
        var h = Math.floor(seconds / 3600)
        var m = Math.floor((seconds % 3600) / 60)
        var s = Math.floor(seconds % 60)
        return String(h).padStart(2, "0") + ":"
             + String(m).padStart(2, "0") + ":"
             + String(s).padStart(2, "0")
    }

    component Card : Rectangle {
        default property alias cardData: inner.data
        Layout.fillWidth: true
        color: root.cCard
        radius: 6
        border.color: root.cBorder
        border.width: 1
        implicitHeight: inner.implicitHeight + 10
        ColumnLayout {
            id: inner
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 5 }
            spacing: 3
        }
    }

    // Компактные варианты контролов: единый мелкий шрифт и минимальные отступы.
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
            spacing: 6

            // ── Заголовок + селектор режима ────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Label {
                    // Имя вкладки говорит, что это отладочный просмотр, а не рабочий
                    // экран: настройки и запуск живут в других местах.
                    text: "Дебаг: развёртка рецепта режима"
                    color: root.cText
                    font.pointSize: 11
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

            // ── Живое состояние насосов и датчиков ───────────────────────
            //
            // НАСТРОЕК ЗДЕСЬ БОЛЬШЕ НЕТ. Они переехали в RegimeSetup —
            // страницу, которая открывается по кнопке режима в RunTable и
            // настраивает конкретную строку очереди; старт/пауза/стоп — на
            // самой RunTable. Эта вкладка отвечает только на вопрос «что сейчас
            // происходит», и органы управления в ней только мешали бы: решение
            // «что запустить» принимается по очереди, а не по развёртке рецепта.
            Card {
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Насосы и датчики"
                        color: root.cBorder
                        font.bold: true
                        font.pointSize: 10
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "⟲ Сброс развёртки"
                        font.pointSize: 9
                        flat: true
                        // Чисто отладочное действие: гасит состояние узлов прошлого
                        // прогона, чтобы следующий читался с чистого листа.
                        enabled: !RegimeTaskTree.running
                        onClicked: root.mon.reset()
                    }
                    Cap { text: root.fixedTimings; font.pointSize: 8 }
                    Label {
                        text: RegimeTaskTree.running
                              ? (RegimeTaskTree.paused ? "на паузе" : "выполняется")
                              : "остановлен"
                        color: RegimeTaskTree.running ? root.stateColor(1) : root.cSub
                        font.pointSize: 9
                        font.bold: true
                    }
                }


                // ── Турбо-этап 12.2: обязательная индикация ────────────────────
                //
                // Без этих полей оператор не может ни принять решение, которого
                // от него требует ТЗ, ни понять, почему режим остановился:
                //   активный насос — единственный видимый признак того, чем
                //     сейчас качают (К176 и К179 одновременно открытыми быть не
                //     должны, REQ-008);
                //   показания с признаком качества — over range это ожидание,
                //     а не ошибка, и путать их нельзя (REQ-079/081);
                //   прогресс удержания — иначе 60 с гейта неотличимы от зависания.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Cap { text: "Насос:" }
                    Rectangle {
                        Layout.preferredWidth: 108
                        Layout.preferredHeight: 22
                        radius: 4
                        color: root.mon.activePump === "turbo" ? "#1f6f3f"
                             : root.mon.activePump === "fore"  ? "#3a3a52"
                                                               : "transparent"
                        border.color: root.cBorder
                        Label {
                            anchors.centerIn: parent
                            font.pointSize: 9
                            font.bold: true
                            color: root.cText
                            text: root.mon.activePump === "turbo" ? "турбо К179"
                                : root.mon.activePump === "fore"  ? "форвакуум К176"
                                                                  : "оба закрыты"
                        }
                    }

                    Cap { text: "ДВ301:" }
                    Label {
                        font.pointSize: 9
                        color: root.mon.dv301Quality === "валидно" ? root.cText : "#d0a050"
                        text: root.fmtPa(root.mon.dv301Pa) + " (" + root.mon.dv301Quality + ")"
                    }

                    Cap { text: "ДВ302:" }
                    Label {
                        font.pointSize: 9
                        color: root.mon.dv302Quality === "валидно" ? root.cText : "#d0a050"
                        text: root.fmtPa(root.mon.dv302Pa) + " (" + root.mon.dv302Quality + ")"
                    }

                    Item { Layout.fillWidth: true }

                    // Прогресс турбо-этапа: до переключения — набор гейта, после
                    // переключения — сама откачка. Без второй стадии полоса
                    // замирала бы на нуле сразу после перехода на турбонасос, и
                    // идущая откачка была бы неотличима от зависшего режима.
                    Cap {
                        visible: root.mon.turboStage.length > 0
                        text: root.mon.turboStage === "gate"
                              ? "гейт ≤ " + root.fmtPa(root.mon.turboGatePa) + ": "
                                + root.mon.turboProgressSec + " / "
                                + root.mon.turboProgressMaxSec + " с"
                              : "турбо-откачка: " + root.fmtSec(root.mon.turboProgressSec)
                                + " / " + root.fmtSec(root.mon.turboProgressMaxSec)
                    }
                    ProgressBar {
                        visible: root.mon.turboStage.length > 0
                        Layout.preferredWidth: 120
                        from: 0
                        to: Math.max(1, root.mon.turboProgressMaxSec)
                        value: root.mon.turboProgressSec
                    }
                }

            }

            // ── SOAK (дебаг): напуск и натекание ──────────────────────────
            //
            // Напуск газа и натекание в камеру — логика SOAK: после откачки
            // идёт напуск, затем выдержка с контролем натекания. Ни в настройках
            // «Вакуума», ни на рабочем экране им места нет: воркера SOAK ещё нет,
            // и пока это отладочный стенд для двух его будущих шагов, а не
            // готовая настройка режима. Место такого стенда — отладочная
            // вкладка, рядом с развёрткой рецепта.
            //
            // ЗАДЕЛ ПОД SOAK: когда появится его воркер, карточка переезжает
            // в RegimeSetup целиком вместе с обработчиками — переносить элементы
            // второй раз не придётся. Сами рецепты (SupplyTaskTree,
            // LeakageTaskTree), их регистрация в RegimeTaskTree::buildRegimeGroup() и
            // пункты меню «Добавить» в RunTable.qml не затронуты — это перенос
            // элементов интерфейса, а не удаление функциональности. Ручные
            // страницы PInlet.qml и LeakageAdjust.qml остаются как есть.
            Card {
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "SOAK (дебаг): напуск и натекание"
                        color: root.cBorder
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Cap {
                        text: "режимы очереди RunTable · применяются сразу"
                    }
                }

                // Режимы добавляются в очередь через меню «Добавить» в RunTable;
                // здесь задаются параметры их прогона. Гейты цепочки (тракт
                // откачан, накопитель заряжен) проверяются рецептами и в UI не
                // настраиваются.
                //
                // Параметры уходят в C++ СРАЗУ, в обработчиках полей: кнопки
                // «Старт» здесь больше нет (она на RunTable), и пушить их на ней
                // было бы некому.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Cap { text: "Напуск: порт" }
                    TextField {
                        Layout.preferredWidth: 64
                        text: root.supplyPort
                        onEditingFinished: { root.supplyPort = text; root.pushSupply() }
                    }
                    Cap { text: "время, с" }
                    Num {
                        from: 1; to: 3600
                        value: root.supplyOpenTimeSec
                        onValueModified: { root.supplyOpenTimeSec = value; root.pushSupply() }
                    }
                    Cap { text: "до, бар" }
                    TextField {
                        Layout.preferredWidth: 72
                        text: root.supplyLimitBar
                        onEditingFinished: {
                            var v = parseFloat(text.replace(",", "."))
                            if (!isNaN(v) && v > 0)
                                root.supplyLimitBar = v
                            text = root.supplyLimitBar
                            root.pushSupply()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Cap { text: "Натекание: клапан" }
                    TextField {
                        Layout.preferredWidth: 64
                        text: root.leakageValve
                        onEditingFinished: { root.leakageValve = text; root.pushLeakage() }
                    }
                    Cap { text: "длительность, с" }
                    Num {
                        from: 1; to: 36000
                        value: root.leakageDurationSec
                        onValueModified: { root.leakageDurationSec = value; root.pushLeakage() }
                    }
                    Cap { text: "или Δp, бар" }
                    TextField {
                        Layout.preferredWidth: 72
                        text: root.leakageTargetDeltaBar
                        ToolTip.text: "0 — не использовать перепад как условие остановки"
                        ToolTip.visible: hovered
                        onEditingFinished: {
                            var v = parseFloat(text.replace(",", "."))
                            if (!isNaN(v) && v >= 0)
                                root.leakageTargetDeltaBar = v
                            text = root.leakageTargetDeltaBar
                            root.pushLeakage()
                        }
                    }
                    Item { Layout.fillWidth: true }
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
                    rowSpacing: 4
                    Label { text: "Повтор:"; color: root.cSub }
                    Label { text: root.mon.currentRepeat + " / " + root.mon.totalRepeats; color: root.cText }
                    Label { text: "Выдержка, с:"; color: root.cSub }
                    Label { text: root.mon.elapsedSec; color: root.cText }
                    Label { text: "Готово повторов:"; color: root.cSub }
                    Label { text: root.mon.repeatsDone; color: root.stateColor(2) }
                    Label { text: "С ошибкой:"; color: root.cSub }
                    Label { text: root.mon.repeatsError; color: root.stateColor(3) }
                }

                // ── Бюджет времени прогона (T_total строки RunTable) ───────────
                //
                // Показывается СУММАРНОЕ время прогона и остаток, а не только
                // прогресс текущего этапа: на финальной откачке важно именно
                // «сколько осталось до конца строки». Это то же число, что
                // стоит в столбце времени RunTable.
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    visible: root.mon.budgetLimited

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label { text: "Время прогона:"; color: root.cSub }
                        Label {
                            text: root.formatHms(root.mon.budgetElapsedSec)
                                  + " / " + root.formatHms(root.mon.budgetTotalSec)
                            color: root.cText
                            font.family: "Consolas"
                        }
                        Item { Layout.fillWidth: true }
                        Label { text: "осталось:"; color: root.cSub }
                        Label {
                            text: root.formatHms(root.mon.budgetRemainingSec)
                            font.family: "Consolas"
                            font.bold: true
                            // Красное на нуле: исчерпанный бюджет прекращает
                            // прогон, и это должно быть видно без чтения лога.
                            color: root.mon.budgetRemainingSec === 0 ? root.stateColor(3)
                                 : root.mon.budgetRemainingSec < root.mon.budgetTotalSec * 0.1
                                   ? "#d08770" : root.cText
                        }
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: Math.max(1, root.mon.budgetTotalSec)
                        value: root.mon.budgetElapsedSec
                    }
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
