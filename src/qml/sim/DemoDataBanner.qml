import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Grams.simSingleton 1.0

// Плашка демо-режима. Загружается Main.qml только при запуске с --sim.
//
// Прогон идёт (machine != idle) — крупная красная плашка с профилем и фазой.
// Прогона нет, но источник подменён — узкая полоса: значения на экране всё
// равно не с датчиков, оператор должен это видеть.
// Демо запрещено (подключено железо) — полоса с причиной.
Rectangle {
    id: banner

    readonly property bool running: SimStatus.demoActive

    implicitHeight: running ? 40 : 22
    color: !SimStatus.allowed ? "#8d6e63"
         : running ? (SimStatus.paused ? "#ef6c00" : "#c62828")
         : "#6d4c41"
    opacity: 0.93

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 16

        Label {
            text: !SimStatus.allowed ? qsTr("ДЕМО-РЕЖИМ ЗАПРЕЩЁН")
                : banner.running ? qsTr("ДЕМО-ДАННЫЕ")
                : qsTr("ДЕМО-ИСТОЧНИК: показания не с датчиков")
            color: "white"
            font.bold: true
            font.pixelSize: banner.running ? 20 : 13
        }
        Label {
            visible: banner.running
            text: SimStatus.profileName
            color: "white"
            elide: Text.ElideRight
            Layout.maximumWidth: 480
        }
        Label {
            visible: banner.running && SimStatus.phaseId !== ""
            text: qsTr("фаза: %1").arg(SimStatus.phaseLabel)
            color: "white"
        }
        Label {
            visible: banner.running && SimStatus.paused
            text: qsTr("ПАУЗА")
            color: "white"
            font.bold: true
        }
        Label {
            visible: SimStatus.lastError !== ""
            text: qsTr("ошибка: %1").arg(SimStatus.lastError)
            color: "white"
        }
        Item { Layout.fillWidth: true }
        Label {
            text: !SimStatus.allowed ? SimStatus.notAllowedReason
                : SimStatus.rpcListening ? qsTr("RPC 127.0.0.1:%1").arg(SimStatus.rpcPort)
                : qsTr("RPC не запущен")
            color: "white"
            font.pixelSize: 12
        }
    }
}
