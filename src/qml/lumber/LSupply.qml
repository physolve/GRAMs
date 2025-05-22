import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lSupply
    color: "#464646"
    border.color: "#594E74"
    anchors.fill: parent
    signal playInlet()
    Button{
        id: supplyBtn
        x: 25
        y: 50
        width: parent.width-50
        height: 50
        text: "Подача газа"
        onClicked: lSupply.playInlet()
    }
    Row{
        x: 25
        y: 130
        spacing: 10
        Text{
            // width: parent.width
            text: "Целевое (индикатор)"
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
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }
}