import QtQuick
import QtQuick.Controls
//import QtQuick.Controls.Material
Rectangle {
    id: sensor
    width: 138
    height: 48
    radius: 14
    color: "#a6bfb3"
    property double value: 0.0012
    Row{
        anchors.fill: sensor
        anchors.topMargin: 4
        anchors.leftMargin: 3
        spacing: 3
        Rectangle {
            id: display
            width: 80
            height: 40
            radius: 10
            color: "#61736c"
            //anchors.top: parent.top
            //anchors.left: parent.left
            // anchors.margins: 2
            Label {
                id: label
                text: Number(sensor.value.toPrecision(5))
                font.pixelSize: 17
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        ShortComboBox{
            model: ["°C", "K"]
            width: 48
            anchors.verticalCenter: parent.verticalCenter
        }
    }

//            RoundButton {
//                id: button
//                width: sensor.width - display.width
    //                height: sensor.height
    //            }

}
