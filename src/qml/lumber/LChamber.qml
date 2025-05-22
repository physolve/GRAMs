import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lChamber
    color: "#464646"
    border.color: "#79A6A4"
    anchors.fill: parent
    signal pickChamber()
    signal setSample()
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
     Button{
        id: samplePickBtn
        x: 5
        y: 150
        width: parent.width-10
        height: 50
        text: "Образец и тигель"
        onClicked: lChamber.setSample()
    }
    
}