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
    // Формат один на оба прибора: два давления рядом должны читаться
    // одинаково, иначе их нельзя сравнить глазом.
    function fmtBar(v) {
        if (!isFinite(v) || v <= 0)
            return "—"
        return v < 1e-2 ? v.toExponential(2) : v.toFixed(2)
    }
    // Логарифмическая шкала 1e-7…1 бар, как было у единственного датчика.
    function logScale(v) {
        if (!isFinite(v) || v <= 0)
            return 0
        return Math.max(0, Math.min(1, (Math.log10(v) + 7) / 7))
    }

    // Два датчика стоят рядом и в одном масштабе: переход на турбонасос
    // виден только по расхождению ДВ301 и ДВ302, а не по одному из них.
    Row {
        anchors.centerIn: parent
        spacing: Grams.guiPres.hasVT ? 12 : 0

        Column {
            spacing: 2
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "ДВ301"
                color: "#FAE0CF"
                font.pointSize: 9
                font.bold: true
            }
            Rectangle {
                id: sliderPlacer1
                width: Grams.guiPres.hasVT ? 96 : 120
                height: width
                radius: width / 2
                color: "lightgray"
                CircularSlider {
                    anchors.centerIn: parent
                    id: slider1 // форвакуум
                    diameter: parent.width + 40
                    progressColor: "#56BF66"
                    minValue: 0
                    rotation: 180
                    progressWidth: 8
                    startAngle: 40
                    trackWidth: 13
                    value: lVacuum.logScale(Grams.guiPres.prARV)
                    maxValue: 1
                    endAngle: 320
                    tickCount: 9
                }
                Label {
                    color: "black"
                    font.family: "Verdana"
                    text: `<p></p><p>${lVacuum.fmtBar(Grams.guiPres.prARV)} <i>бар</i></p>`
                    anchors.centerIn: parent
                    horizontalAlignment: Text.AlignHCenter
                    font.pointSize: sliderPlacer1.width > 100 ? 14 : 11
                }
            }
        }

        // ДВ302 показывается, ТОЛЬКО если прибор есть в профиле: пустой
        // циферблат читался бы как глубокий вакуум в турбо-тракте.
        Column {
            visible: Grams.guiPres.hasVT
            spacing: 2
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "ДВ302 турбо"
                color: "#ABDBDD"
                font.pointSize: 9
                font.bold: true
            }
            Rectangle {
                id: sliderPlacer2
                width: 96
                height: width
                radius: width / 2
                color: "lightgray"
                CircularSlider {
                    anchors.centerIn: parent
                    id: slider2 // турбо-тракт
                    diameter: parent.width + 40
                    progressColor: "#3AA6B9"
                    minValue: 0
                    rotation: 180
                    progressWidth: 8
                    startAngle: 40
                    trackWidth: 13
                    value: lVacuum.logScale(Grams.guiPres.prVT)
                    maxValue: 1
                    endAngle: 320
                    tickCount: 9
                }
                Label {
                    color: "black"
                    font.family: "Verdana"
                    text: `<p></p><p>${lVacuum.fmtBar(Grams.guiPres.prVT)} <i>бар</i></p>`
                    anchors.centerIn: parent
                    horizontalAlignment: Text.AlignHCenter
                    font.pointSize: 11
                }
            }
        }
    }
}