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
                    "value": TimeStamp.guiPresVirtual.prSQ
                },
                {   
                    "name": "RQ",
                    "value": TimeStamp.guiPresVirtual.prRQ
                },
                {
                    "name": "SC1",
                    "value": TimeStamp.guiPresVirtual.prSC1
                },
                {
                    "name": "SC2",
                    "value": TimeStamp.guiPresVirtual.prSC2
                },
                {
                    "name": "SC3",
                    "value": TimeStamp.guiPresVirtual.prSC3
                },
                {
                    "name": "SB",
                    "value": TimeStamp.guiPresVirtual.prSB
                },
                {
                    "name": "SD1",
                    "value": TimeStamp.guiPresVirtual.prSD1
                },
                {
                    "name": "RE",
                    "value": TimeStamp.guiPresVirtual.prRE
                },
                {
                    "name": "RD2",
                    "value": TimeStamp.guiPresVirtual.prRD2
                },
                {
                    "name": "RF",
                    "value": TimeStamp.guiPresVirtual.prRF
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