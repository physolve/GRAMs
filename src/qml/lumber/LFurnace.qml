import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 2.12

Rectangle {
    id: lFurnace
    color: "#464646"
    border.color: "#E6BDA1"
    anchors.fill: parent
    signal setFurnace()
    Button{
        id: chamberPickBtn
        x: 5
        y: 5
        width: parent.width-10
        height: 50
        text: "Настройки печи"
        // onClicked: lChamber.pickChamber()
    }
    Button{
        y: 60
        x: parent.width/2-20
        width: 40
        height: 50
        background: Rectangle {
            color: "#56BF66"
            radius: 0
            
        }
    }
    Button{
        y: 110
        x: parent.width/2-20
        width: 40
        height: 50
        background: Rectangle {
            color: "#F2C029"
            radius: 0
        }
    }
    // Button{
    //     y: 145
    //     x: parent.width/2-20
    //     width: 40
    //     height: 40
    //     background: Rectangle {
    //         color: "#D94625"
    //         radius: 10
    //     }
    // }
}