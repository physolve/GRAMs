import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Style
import CustomPlot

Page{
    id: page1
    title: qsTr("Page 1")

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        RowLayout{
            GRAMsMnemoForm {
                id: gRAMsMnemoForm
                width: 1350
                height: 900
            }
            ColumnLayout {
                Layout.preferredWidth: 490
                Layout.fillHeight: true
                ListView {
                    id: dataView
                    model: _myModel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    interactive: false
                    delegate: Item {
                        width: 480
                        height: 180
                        CustomPlotItem {
                            id: customPlot
                            width: parent.width;  height: parent.height // resize
                            Component.onCompleted: initCustomPlot(model.index)
                            Component.onDestruction: testJSString(model.index)
                            function testJSString(num) {
                                var text = "I have been destroyed_ %1"
                                console.log(text.arg(num))
                            }
                        }
                        Connections {
                            target: dataView.model    // EDIT: I drew the wrong conclusions here, see text below!
                            function onDataChanged() {
                                customPlot.backendData(model.x, model.y)
                                //gRAMsMnemoForm.setVal(model.cv, model.index) // it's Working
                            }
                        }
                    }
                    focus: true
                    orientation: ListView.Vertical
                    ScrollBar.vertical: ScrollBar {
                    active: true
                    }
                }

                // Connections {
                //     target: dataView.model    // EDIT: I drew the wrong conclusions here, see text below!
                //     function onDataChanged() {
                //         gRAMsMnemoForm.newVal = model.cv // it's Working
                //     }
                // }
            }
        }
    }
    Connections {
        target: _myModel
        onDataChanged: {
            gRAMsMnemoForm.setPressureVal(_myModel.getCurPressureValues()) // it's Working fine
            gRAMsMnemoForm.setTempVal(_myModel.getCurTempValues())
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