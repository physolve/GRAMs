import QtQuick
import QtQuick.Controls
//import QtQuick.Controls.Material
Rectangle {
    id: sensor
    width: 86
    height: 80
    radius: 14
    color: "#a6bfb3"
    property double value: 0.0012
    Column{
        anchors.fill: sensor
        anchors.topMargin: 4
        anchors.leftMargin: 3
        spacing: 5
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
        Row {
            spacing: 4
            // anchors.bottom: sensor.bottom
            // anchors.horizontalCenter: sensor.horizontalCenter
            CheckBox {
                id: control
                width: 25
                height: 25
                // anchors.verticalCenter: parent.verticalCenter
                indicator: Rectangle {
                    implicitWidth: 25
                    implicitHeight: 25
                    radius: 5
                    border.color: control.checked ? "red" : "#17a81a"
                    anchors.verticalCenter: parent.verticalCenter
                    color: control.checked ? "black" : "white "
                    Text {
                        width: 25
                        height: 25
                        text: "e"
                        // font.family: "Futura Md BT"
                        font.italic: true
                        font.pointSize: 12
                        // opacity: control.checked ? 1.0 : 0.3
                        color: control.checked ? "white" : "black"
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            ShortComboBox{
                model: ["Бар", "Па"]
                width: 52
            }
        }
    }

//            RoundButton {
//                id: button
//                width: sensor.width - display.width
    //                height: sensor.height
    //            }

}
