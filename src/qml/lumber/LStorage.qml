import QtQuick
import QtQuick.Controls
// import QtQuick.Layouts
import "content"

Item {
    id: root
    // color: "#2B2B2B"
    anchors.fill: parent
    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 10
        color: "White"
        font.family: "Verdana"
        text: "Эталонный резервуар" // changing to exp function
        // anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 15
    }
    Rectangle {
        id: sliderPlacer1
        width: 120
        height: 120
        radius: 60
        color: "lightgray"
        anchors.centerIn: parent
        CircularSlider {
            anchors.centerIn: parent
            id: slider1 // pressure
            diameter: 200
            progressColor: "#55C1D9"
            minValue: 0
            rotation: 180
            progressWidth: 8
            startAngle: 40
            trackWidth: 13
            value: 3
            maxValue: 50
            endAngle: 320
        }
        CircularSlider {
            anchors.centerIn: parent
            id: slider2 // pressure
            diameter: 160
            progressColor: "#56BF66"
            minValue: 0
            rotation: 180
            progressWidth: 8
            startAngle: 40
            trackWidth: 13
            value: 3
            maxValue: 2
            endAngle: 320
        }
        Label {
            // width: 40
            // height: 20
            color: "black"
            font.family: "Verdana"
            text: `<p></p><p>${slider1.value.toFixed(3)} <i>бар</i></p><p>25 <i>°C</i></p>` // changing to exp function
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 15
        }
    }
}