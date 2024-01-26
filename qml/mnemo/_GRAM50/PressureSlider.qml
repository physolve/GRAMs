import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import ".."
CircularSlider {
    id: pres_slider
    width: 80
    height: 80
    onValueChanged: item1.newVal = value
    handle: Rectangle {
        id: handleItem
        width: 6
        height: 6
        color: "#908990"
        radius: width / 2
        border.color: "#fefefe"
        border.width: 5
        antialiasing: true
        transform: [
            Translate {
                x: (pres_slider.handleWidth - width) / 2
                y: (pres_slider.handleHeight - height) / 2
            }
        ]
    }
    minValue: 0
    rotation: 270
    progressWidth: 10
    startAngle: 40
    Layout.fillWidth: false
    trackWidth: 10
    value: 60
    maxValue: 120
    endAngle: 320

    Label {
        width: 40
        height: 20
        color: "#fefefe"
        text: "value"
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        rotation: 90
        font.pointSize: 13
    }
}
