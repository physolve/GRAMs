import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lChamber
    color: "#464646"
    border.color: "#79A6A4"
    anchors.fill: parent
    signal pickChamber()
    Button{
        id: chamberPickBtn
        x: 5
        y: 5
        width: parent.width-10
        height: 50
        text: "Выбрать камеру"
        onClicked: lChamber.pickChamber()
    }
    Text{
        width: parent.width
        y: 80
        text: "Камера ..."
        font.family: "Verdana"
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 14
        color: "white"
    }
    Text{
        width: parent.width
        y: 130
        text: "Целевое давление"
        font.family: "Verdana"
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 12
        color: "white"
    }
    TextField{
        x: 5
        width: parent.width - 10
        y: 170
        readOnly: true
        text: "0"
        font { family: 'Courier'; pixelSize: 16; }
        horizontalAlignment: TextInput.AlignHCenter
        selectByMouse: true
    }
    
}