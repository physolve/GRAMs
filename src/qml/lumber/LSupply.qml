import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lSupply
    color: "#464646"
    border.color: "#594E74"
    anchors.fill: parent
    Button{
        id: supplyBtn
        x: 25
        y: 50
        width: parent.width-50
        height: 50
        text: "Подача газа"
        // onClicked: lChamber.pickChamber()
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
        x: 40
        width: parent.width - 80
        y: 170
        readOnly: true
        text: "0"
        font { family: 'Courier'; pixelSize: 16; }
        horizontalAlignment: TextInput.AlignHCenter
        selectByMouse: true
    }
}