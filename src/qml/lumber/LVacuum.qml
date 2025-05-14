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
    Row{
        x: 25
        y: 65
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