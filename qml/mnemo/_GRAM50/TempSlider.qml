import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Studio.Effects
import ".."
CircularSlider {
    id: t_slider
    hideProgress: true
    hideTrack: true
    width: 80
    height: 80
    //onValueChanged: item1.newTempVal = value
    handleColor: "#6272A4"
    handleWidth: 10
    handleHeight: 10
    value: 0.5
    // Custom progress Indicator
    Item {
        anchors.fill: parent
        anchors.margins: 5
        Rectangle {
            id: mask
            anchors.fill: parent
            radius: width / 2
            color: "#282A36"
            border.width: 5
            border.color: "#44475A"
        }

        Item {
            anchors.fill: mask
            anchors.margins: 5
            layer.enabled: true
            rotation: 180
            layer.effect: OpacityMaskEffect {
                id: opacityMask
                maskSource: mask
            }
            Rectangle {
                height: parent.height * t_slider.value
                width: parent.width
                //radius: parent.width/2
                layer.enabled: true
                color: "#ec0636"
            }
        }

        Label {
            anchors.centerIn: parent
            font.pointSize: 8
            color: "#FEFEFE"
            text: "value"
        }
    }
}
