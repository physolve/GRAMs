import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lTools
    color: "#464646"
    border.color: "#F2C029"
    anchors.fill: parent
    // reminder to change for other systems
    signal openESupply()
    Button{
        x: 25
        y: 25
        id: expSettingsBtn
        width: 200
        height: 50
        text: "Автоподача до камеры"
        onClicked: lTools.openESupply()
    }
}