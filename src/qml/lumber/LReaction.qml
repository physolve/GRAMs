import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "content"

Item {
    id: root
    // color: "#2B2B2B"
    anchors.fill: parent
    // valve states
    property double testVal: 1
    Rectangle {
        id: sliderPlacer
        width: 120
        height: 120
        radius: 60
        color: "lightgray"
        anchors.centerIn: parent
        CircularSlider {
            anchors.centerIn: parent
            id: slider1 // pressure
            diameter: 240
            progressColor: "#55C1D9"
            minValue: 0
            rotation: 180
            progressWidth: 8
            startAngle: 40
            trackWidth: 13
            value: testVal
            maxValue: 50
            endAngle: 320
        }
        CircularSlider {
            anchors.centerIn: parent
            id: slider2 // pressure
            diameter: 200
            progressColor: "#56BF66"
            minValue: 0
            rotation: 180
            progressWidth: 8
            startAngle: 40
            trackWidth: 13
            value: testVal
            maxValue: 2
            endAngle: 320
        }
        CircularSlider {
            anchors.centerIn: parent
            id: slider3 // pressure
            diameter: 160
            progressColor: "#F2C029"
            minValue: 0
            rotation: 180
            progressWidth: 8
            startAngle: 40
            trackWidth: 13
            value: (Math.log10(testVal)+7)/7
            maxValue: 1
            endAngle: 320
            tickCount: 9
        }
        Label {
            // width: 40
            // height: 20
            color: "black"
            font.family: "Verdana"
            property var myValue: (slider1.value < 1e-2) ? 
                    slider1.value.toExponential(2) : slider1.value.toFixed(3)
            text: `<p></p><p>${myValue} <i>бар</i></p><p>25 <i>°C</i></p>` // changing to exp function
            // anchors.centerIn: parent
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 15
        }
    }
}