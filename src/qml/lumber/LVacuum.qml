import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lVacuum
    color: "#464646"
    border.color: "#FAE0CF"
    anchors.fill: parent
    Button{
        id: vacuumBtn
        x: 25
        y: 5
        width: parent.width-50
        height: 50
        text: "Вакуумирование"
        // onClicked: lChamber.pickChamber()
    }
    Text{
        y: 60
        width: parent.width
        text: "Целевое давление"
        font.family: "Verdana"
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 12
        color: "white"
    }
    TextField{
        x: 40
        y: 100
        width: parent.width - 80
        readOnly: true
        text: "0"
        font { family: 'Courier'; pixelSize: 16; }
        horizontalAlignment: TextInput.AlignHCenter
        selectByMouse: true
    }
}