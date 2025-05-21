import QtQuick
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0
import "content"

Rectangle {
    id: lVacuum
    color: "#464646"
    border.color: "#FAE0CF"
    anchors.fill: parent
    signal playVacuum()
    Button{
        id: vacuumBtn
        x: 10
        y: 5
        width: 50
        height: 60
        text: "Вак."
        font.pointSize: 9

        // background: Rectangle {
        //     color: "white"
        //     border.width: 1
        //     border.color: "blue"
        //     radius: parent.width/4
        // }
        // icon.source: "qrc:/vacuumSVG.svg"
        // color: pressed ? "red" : "white"
        onClicked: lVacuum.playVacuum()
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
            diameter: 160
            progressColor: "#56BF66"
            minValue: 0
            rotation: 180
            progressWidth: 8
            startAngle: 40
            trackWidth: 13         
            value: (Math.log10(Grams.guiPres.prARV)+7)/7
            maxValue: 1
            endAngle: 320
            tickCount: 9
        }
        Label {
            // width: 40
            // height: 20
            color: "black"
            font.family: "Verdana"
            property var myValue: (Grams.guiPres.prARV < 1e-2) ? 
                    Grams.guiPres.prARV.toExponential(2) : Grams.guiPres.prARV.toFixed(3)
            text: `<p></p><p>${myValue} <i>бар</i></p>` // changing to exp function
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 15
        }
    }
}