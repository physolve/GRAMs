import QtQuick
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0
import "content"

Item {
    id: root
    // color: "#2B2B2B"
    anchors.fill: parent
    // valve states
    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 10
        color: "White"
        font.family: "Verdana"
        text: "Реакционный резервуар" // changing to exp function
        // anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 15
    }
    Rectangle {
        id: sliderPlacer
        width: 120
        height: 120
        radius: 60
        color: "lightgray"
        anchors.centerIn: parent
        property double reactionPressure: Grams.guiPresVirtual.prRQ > 0 ? Grams.guiPresVirtual.prRQ : 1e-5 
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
            value: parent.reactionPressure
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
            value: parent.reactionPressure
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
            value: (Math.log10(parent.reactionPressure)+7)/7
            maxValue: 1
            endAngle: 320
            tickCount: 9
        }
        Label {
            // width: 40
            // height: 20
            color: "black"
            font.family: "Verdana"
            property string pressureStr: parent.reactionPressure > 0 ? 
                                            parent.reactionPressure < 1e-2 ? parent.reactionPressure.toExponential(1).toString() 
                                                : parent.reactionPressure.toFixed(2).toString()
                                            : "< 1e-5"
            text: `<p></p><p>${pressureStr} <i>бар</i></p><p>25 <i>°C</i></p>` // changing to exp function
            // anchors.centerIn: parent
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 15
        }
    }
}