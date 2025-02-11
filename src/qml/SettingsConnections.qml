import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 2.12

GridLayout{
    // Layout.fillHeight: true
    // Layout.fillWidth: true
    flow: GridLayout.LeftToRight
    columnSpacing: 10
    columns: width/490 
    GroupBox {
        title: qsTr("Hardware")
        Layout.minimumHeight: 350
        Layout.minimumWidth: 490
        Layout.alignment: Qt.AlignTop, Qt.AlignHCenter
        Rectangle { 
            id: lblValveMap
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin:10
            width: 100
            height: 20
            color:"transparent"; border.color: "#464646";
            Text{
                text: "valve map: " 
                font.pointSize: 9; color: "white" 
                anchors.centerIn: parent
            }
        }
        GridView {
            id: viewValveMap
            anchors.top: parent.top
            anchors.left: lblValveMap.right
            anchors.right: parent.right
            height: cellHeight * 3
            flow: GridView.FlowLeftToRight
            anchors.topMargin:10
            anchors.leftMargin: 5
            cellWidth: 60; cellHeight: 20
            model: initSource ? initSource.hardware.valves : ""
            clip: true
            interactive: false
            delegate: Rectangle { 
                width: 60
                height: 20
                color:"transparent"; border.color: "#464646";
                Text { text: index + ". " + modelData; font.pointSize: 9; color: "white"; anchors.centerIn: parent } 
            }
        }
        Rectangle {
            id: lblPressureSensors
            anchors.top: viewValveMap.bottom
            anchors.left: parent.left
            anchors.topMargin:10
            width: 100
            height: 20
            color:"transparent"; border.color: "#464646";
            Text{
                text: "pressure sensors: " 
                font.pointSize: 9; color: "white" 
                anchors.centerIn: parent
            }
        }
        ListView{
            id: viewPressureSensorsMap
            anchors.top: viewValveMap.bottom
            anchors.left: lblPressureSensors.right
            anchors.right: parent.right
            height: 20 * 8
            anchors.topMargin: 10
            anchors.leftMargin: 5
            clip: true
            interactive: false
            orientation: Qt.Vertical
            model: initSource ? Object.keys(initSource.hardware.pressureSensors()) : 0
            delegate: Rectangle { 
                width: 350
                height: 20
                color: "transparent"; border.color: "#464646";
                property var curSensor: initSource.hardware.pressureSensors()[modelData]
                Text { text: `${modelData}, A = ${curSensor.A}, B = ${curSensor.B}, R = ${curSensor.R}, ch = ${curSensor.cch}` ; font.pointSize: 9; color: "white"; anchors.centerIn: parent }
            }
        }
        Rectangle {
            id: lblTemperatureSensors
            anchors.top: viewPressureSensorsMap.bottom
            anchors.left: parent.left
            anchors.topMargin: 10
            width: 100
            height: 20
            color:"transparent"; border.color: "#464646";
            Text{
                text: "temp. sensors: "
                font.pointSize: 9; color: "white" 
                anchors.centerIn: parent
            }
        }
        GridView {
            id: viewTemperatureSensorsMap
            anchors.top: viewPressureSensorsMap.bottom
            anchors.left: lblTemperatureSensors.right
            anchors.right: parent.right
            height: cellHeight * 2
            flow: GridView.FlowLeftToRight
            anchors.topMargin:10
            anchors.leftMargin: 5
            cellWidth: 60; cellHeight: 20
            model: initSource ? initSource.hardware.tempSensors : 0
            delegate: Rectangle { 
                width: 60
                height: 20
                color: "transparent"; border.color: "#464646";
                Text { text: `${index}. ${modelData}` ; font.pointSize: 9; color: "white"; anchors.centerIn: parent }
            }
        }
    }
    GroupBox {
        title: qsTr("Controllers")
        Layout.minimumHeight: 350
        Layout.minimumWidth: 480
        Layout.alignment: Qt.AlignTop, Qt.AlignHCenter
        Rectangle { 
            id: lblAdvantechMap
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin:10
            width: 110
            height: 20
            color:"transparent"; border.color: "#464646";
            Text{
                text: "Advantech map: " 
                font.pointSize: 10; color: "white" 
                anchors.centerIn: parent
            }
            Component.onCompleted:{
                let test = initSource.daq()
                console.log(test.length)
            }
            ListView{
            id: viewDaqMap
            anchors.top: parent.top
            anchors.left: lblAdvantechMap.right
            anchors.right: parent.right
            height: 20 * 8
            anchors.topMargin: 10
            anchors.leftMargin: 5
            clip: true
            interactive: false
            orientation: Qt.Vertical
            model: initSource ? initSource.daq().length : 0
            delegate: Rectangle { 
                width: 350
                height: 20
                color: "transparent"; border.color: "#464646";
                property var curDaq: initSource.daq()[index]
                Text { text: `${curDaq.device}` ; font.pointSize: 9; color: "white"; anchors.centerIn: parent }
            }
        }
        }
    }
}