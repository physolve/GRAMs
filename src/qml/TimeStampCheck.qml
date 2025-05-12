// form for possible SqlModelTable

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.timeStampSingleton 1.0

Item {
    // id: root
    // color: "#2B2B2B"
    height: 300
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
                    "value": TimeStamp.guiPresTimestamp.prSQ
                },
                {   
                    "name": "RQ",
                    "value": TimeStamp.guiPresTimestamp.prRQ
                },
                {
                    "name": "SC1",
                    "value": TimeStamp.guiPresTimestamp.prSC1
                },
                {
                    "name": "SC2",
                    "value": TimeStamp.guiPresTimestamp.prSC2
                },
                {
                    "name": "SC3",
                    "value": TimeStamp.guiPresTimestamp.prSC3
                },
                {
                    "name": "SB",
                    "value": TimeStamp.guiPresTimestamp.prSB
                },
                {
                    "name": "SD1",
                    "value": TimeStamp.guiPresTimestamp.prSD1
                },
                {
                    "name": "RE",
                    "value": TimeStamp.guiPresTimestamp.prRE
                },
                {
                    "name": "RD2",
                    "value": TimeStamp.guiPresTimestamp.prRD2
                },
                {
                    "name": "RF",
                    "value": TimeStamp.guiPresTimestamp.prRF
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