import QtQuick
import QtQuick.Controls

Item {
    id: sensor
    anchors.fill: parent
    property double value: 0.0012
    required property string name 
    Rectangle {
        id: display
        x: 5
        y: 5
        anchors.centerIn: parent
        width: 80
        height: 70
        radius: 10
        color: "lightgray"
        property string formVal: (sensor.value < 1e-2) ? 
                sensor.value.toExponential(2) : sensor.value.toFixed(3)
        Label {
            id: label
            text: `<p></p><p>${sensor.name}</p><p>${parent.formVal}</p><p><i>ммоль/с</i></p>`
            color: "black"
            font.family: "Verdana"
            font.pixelSize: 15
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
