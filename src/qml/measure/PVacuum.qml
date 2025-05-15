import QtQuick
import QtQuick.Controls

Item {
    id: root
    // color: "#2B2B2B"
    Row{
        x: 25
        y: 170
        spacing: 10
        Text{
            // width: parent.width
            text: "Целевое"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            readOnly: true
            text: "0"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
        Text{
            // width: parent.width
            text: ", бар"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
    }
}