import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lTools
    color: "#464646"
    border.color: "#F2C029"
    anchors.fill: parent
    signal openExpSettings()
    Button{
        id: expSettingsBtn
        x: 5
        y: 5
        width: 150
        height: 50
        text: "Параметры эксперимента"
        // onClicked: lChamber.pickChamber()
    }
}