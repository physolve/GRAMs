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
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop, Qt.AlignHCenter
        Rectangle { 
            id: lblValveMap
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin:10
            width: 100
            height: 20
            color:"transparent"; border.color: "#464646";
            Text {
                text: "valve map: " 
                font.pointSize: 9; color: "white"
                font.family: "Verdana" 
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
            cellWidth: 70; cellHeight: 20
            model: initSource.hardware.valves
            clip: true
            interactive: false
            delegate: Rectangle { 
                width: 68
                height: 20
                color:"transparent"; border.color: "#464646";
                Text { text: index + ". " + modelData; font.pointSize: 9; color: "white"; font.family: "Verdana"; anchors.centerIn: parent } 
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
            Text {
                text: "pressure sensors: " 
                font.pointSize: 9; color: "white"
                font.family: "Verdana"
                anchors.centerIn: parent
            }
        }
        ListView {
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
            model: Object.keys(initSource.hardware.pressureSensors()) // rewrite as property
            delegate: Rectangle { 
                width: 350
                height: 20
                color: "transparent"; border.color: "#464646";
                property var curSensor: initSource.hardware.pressureSensors()[modelData]
                Text { text: `${modelData}, A = ${curSensor.A}, B = ${curSensor.B}, R = ${curSensor.R}, ch = ${curSensor.cch}` ; font.pointSize: 9
                color: "white"; font.family: "Verdana"; anchors.centerIn: parent }
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
            Text {
                text: "temp. sensors: "
                font.pointSize: 9; color: "white" 
                anchors.centerIn: parent
                font.family: "Verdana"
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
            cellWidth: 70; cellHeight: 20
            model: initSource.hardware.tempSensors
            delegate: Rectangle { 
                width: 68
                height: 20
                color: "transparent"; border.color: "#464646";
                Text { text: `${index}. ${modelData}` ; font.pointSize: 9; color: "white";
                font.family: "Verdana"; anchors.centerIn: parent }
            }
        }
    }
    GroupBox {
        title: qsTr("Controllers")
        Layout.minimumHeight: 350
        Layout.minimumWidth: 490
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop, Qt.AlignHCenter
        Rectangle { 
            id: lblAdvantechMap
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin:10
            width: 110
            height: 20
            color:"transparent"; border.color: "#464646";
            Text {
                text: "Advantech map: " 
                font.pointSize: 10; color: "white" 
                font.family: "Verdana"
                anchors.centerIn: parent
            }
            Component.onCompleted: {
                let test = initSource.daqGui
                console.log(test.length)
                console.log(test[0].device)
            }
        }
        // function to change parameter fill rectangle 
        ListView {
            id: viewDaqMap
            anchors.top: parent.top
            anchors.left: lblAdvantechMap.right
            anchors.right: parent.right
            height: 20 * 8
            anchors.topMargin: 10
            anchors.leftMargin: 5
            spacing: 5
            clip: true
            interactive: false
            orientation: Qt.Vertical
            model: initSource.daqGui
            delegate: Rectangle { // basically we have modelData - state !!!
                width: 350
                height: 20
                color: modelData.state ? "#36454f" : "transparent"
                border.color: "#464646"
                Text { text: `${modelData.device}, ${modelData.purpose}`; font.pointSize: 9
                color: "white"; font.family: "Verdana"; anchors.centerIn: parent }
            }
        } // I want to highlight connected 
        
        // let's view other things later
        // addRemoveQuarParameters
        // storageQuarParameters
        // reactionQuarParameters
        // secondLineQuarParameters
        // securityParameters
        // now concentrate 
    }
}