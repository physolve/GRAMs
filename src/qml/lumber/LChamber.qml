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
    // Text{
    //     width: parent.width
    //     y: 80
    //     text: "Камера ..."
    //     font.family: "Verdana"
    //     horizontalAlignment: Text.AlignHCenter
    //     font.pointSize: 14
    //     color: "white"
    // }
    
    Row{
        x: 25
        y: 170
        spacing: 10
        Text{
            // width: parent.width
            text: "" //Целевое
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