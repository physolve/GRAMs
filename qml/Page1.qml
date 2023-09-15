import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Style


Page{
    id: page1
    title: qsTr("Page 1")

    StackLayout {
        id: stackLayout
        anchors.fill: parent

        GRAMsMnemoForm {
            id: gRAMsMnemoForm
            width: 1800
            height: 900
        }
    }
    //property alias headerColor: label.color
    // Rectangle {
    //     id: rect2
    //     width: parent.width; height: 30

    //     gradient: Gradient {
    //         GradientStop { position: 0.0; color: "lightsteelblue" }
    //         GradientStop { position: 1.0; color: "slategray" }
    //     }

    //     Label {
    //         id: label
    //         anchors.top: parent.top
    //         anchors.horizontalCenter: parent.horizontalCenter
    //         color: Style.text.color.primary
    //         text: qsTr("Page One")
    //     }
    // }
    // //![1]
    // Item {
    //     id: testQtChart
    //     width: 600
    //     height: 400
    //     anchors.top: rect2.bottom
    //     ControlPanel {
    //         id: controlPanel
    //         anchors.top: parent.top
    //         anchors.topMargin: 10
    //         anchors.bottom: parent.bottom
    //         anchors.left: parent.left
    //         anchors.leftMargin: 10
    // //![1]

    //         onSignalSourceChanged: {
    //             if (source == "sin")
    //                 dataSource.generateData(0, signalCount, sampleCount);
    //             else
    //                 dataSource.generateData(1, signalCount, sampleCount);
    //             myChart.axisX().max = sampleCount;
    //         }
    //         onSeriesTypeChanged: myChart.changeSeriesType(type);
    //         onRefreshRateChanged: myChart.changeRefreshRate(rate);
    //         onAntialiasingEnabled: myChart.antialiasing = enabled;
    //         onOpenGlChanged: {
    //             myChart.openGL = enabled;
    //         }
    //     }

    // //![2]
    //     MyChart {
    //         id: myChart
    //         anchors.top: parent.top
    //         anchors.bottom: parent.bottom
    //         anchors.right: parent.right
    //         anchors.left: controlPanel.right
    //         height: testQtChart.height
    //     }

    // }
    // //![2]
    // RowLayout{
    //     anchors.fill: parent
    //     // Switch {
    //     //     id: startChart
    //     //     text: qsTr("Start timer")
    //     //     onToggled: {
    //     //         myChart.startTimer(startChart.checked);
    //     //     }
    //     // }
    // }

}