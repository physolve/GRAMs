// form for possible SqlModelTable

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.timeStampSingleton 1.0

Item {
    // id: root
    // color: "#2B2B2B"
    height: 230
    width: 400
    GroupBox {
        title: TimeStamp.checkStampTime.toISOString()//toUTCString()
        height: parent.height
        width: parent.width
        ListView {
            id: viewPressureTimeStamp
            height: parent.height
            width: parent.width
            clip: true
            interactive: false
            orientation: Qt.Vertical
            property var timeStampMap: [
                {
                    "name": "SQ",
                    "value": TimeStamp.guiPresTimestamp.prSQ.toFixed(3)
                },
                {   
                    "name": "RQ",
                    "value": TimeStamp.guiPresTimestamp.prRQ.toFixed(3)
                },
                {
                    "name": "SC1",
                    "value": TimeStamp.guiPresTimestamp.prSC1.toFixed(3)
                },
                {
                    "name": "SC2",
                    "value": TimeStamp.guiPresTimestamp.prSC2.toFixed(3)
                },
                {
                    "name": "SC3",
                    "value": TimeStamp.guiPresTimestamp.prSC3.toFixed(3)
                },
                {
                    "name": "SB",
                    "value": TimeStamp.guiPresTimestamp.prSB.toFixed(3)
                },
                {
                    "name": "SD1",
                    "value": TimeStamp.guiPresTimestamp.prSD1.toFixed(3)
                },
                {
                    "name": "RE",
                    "value": TimeStamp.guiPresTimestamp.prRE.toFixed(3)
                },
                {
                    "name": "RD2",
                    "value": TimeStamp.guiPresTimestamp.prRD2.toFixed(3)
                },
                {
                    "name": "RF",
                    "value": TimeStamp.guiPresTimestamp.prRF.toFixed(3)
                }
            ]
            model: timeStampMap // rewrite as property
            delegate: Rectangle { 
                width: 150
                height: 20
                color: "transparent"; border.color: "#464646";
                Text { text: `${modelData.name}, ${modelData.value}`; font.pointSize: 9
                color: "white"; font.family: "Verdana"; anchors.centerIn: parent }
            }
        }
    }
}