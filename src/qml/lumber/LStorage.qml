import QtQuick
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0
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
        property double storagePressure: Grams.guiPresVirtual.prSQ > 0 ? Grams.guiPresVirtual.prSQ : 1e-5
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
            value: parent.storagePressure
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
            value: parent.storagePressure
            maxValue: 2
            endAngle: 320
        }
        Label {
            // width: 40
            // height: 20
            color: "black"
            font.family: "Verdana"
            property string pressureStr: parent.storagePressure > 0 ? 
                                            parent.storagePressure < 1e-2 ? parent.storagePressure.toExponential(1).toString() 
                                                : parent.storagePressure.toFixed(2).toString()
                                            : "< 1e-5"
            text: `<p></p><p>${pressureStr} <i>бар</i></p><p>25 <i>°C</i></p>` // changing to exp function
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 15
        }
    }
}