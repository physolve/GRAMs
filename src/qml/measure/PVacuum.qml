import QtQuick
import QtQuick.Controls

Rectangle { 
    id: lblValveMap
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.topMargin:10
    width: 100
    height: 20
    color:"transparent"; border.color: "#464646";
    Text {
        text: "valve map: " 
        font.pointSize: 9; color: "white"
        font.family: "Verdana" 
        anchors.centerIn: parent
    }
}