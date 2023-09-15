import QtQuick
import QtQuick.Controls
import QtQml.Models
import CustomPlot
import Style
// Page with example Action, Buttons, Dialog, page loading, State change, Animation etc.
Page {
    id: page2
    title: qsTr("Page 2")
    Rectangle {
        id: rect1
        width: parent.width; height: 30

        gradient: Gradient {
            GradientStop { position: 0.0; color: "lightsteelblue" }
            GradientStop { position: 1.0; color: "slategray" }
        }

        Label {
            id: label
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            color: Style.text.color.primary
            text: qsTr("Page Two")
        }
    }
    SplitView {
        id: splitView
        anchors.fill: parent
        orientation: Qt.Vertical
        Rectangle {
            implicitWidth: 200
            SplitView.maximumHeight: 400
            color: "lightblue"
            ListView {
                id: view
                anchors.fill: parent
                model: dataModel
                delegate: Text {
                    text: name + " " + value
                }
            }
        }
        Rectangle {
            id: centerItem
            SplitView.minimumHeight: 50
            SplitView.fillWidth: true
            color: "lightgray"
            Label {
                text: "View 2"
                anchors.centerIn: parent
            }
        }
        Rectangle {
            implicitHeight: 200
            color: "lightgreen"
            Row {
                    id: root
                    width: parent.width
                    height: parent.height-rect1.height
                    spacing: 2
                    anchors.top: rect1.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right

                    LineChart {
                        id: lineChart
                        width: root.width/2;  height: root.height // resize

                        title:  'Pendulum Position versus Time'
                        yLabel: 'position (degrees)'
                        xLabel: 'time (s)'
                        color:  'red'
                    }
                    // CustomPlotItem {
                    //     id: customPlot
                    //     width: root.width/2;  height: root.height // resize

                    //     Component.onCompleted: initCustomPlot(-1)
                    // }
            }
        }
        Rectangle {
            id: storeData
            width: parent.width 
            SplitView.maximumHeight: 100
            Button {
                    id: testJSONbtn
                    anchors.top: storeData.bottom
                    text: qsTr("test JSON");
                    onClicked: {
                        var num = Math.round(Math.random() * 10)
                        dataModel.append({ "name": "test" + num, "value": num })
                    }
            }
        }
        
    }

    // ListView {
    //     id: root
        
    //     width: 600
    //     height: 400
    //     anchors.top: rect1.bottom
        
    //     model: ObjectModel { // VisualItemModel
            
    //         LineChart {
    //             id: lineChart
    //             width: root.width;  height: root.width // resize

    //             title:  'Pendulum Position versus Time'
    //             yLabel: 'position (degrees)'
    //             xLabel: 'time (s)'
    //             color:  'red'
    //         }

    //         CustomPlotItem {
    //             id: customPlot
    //             anchors.fill: parent

    //             Component.onCompleted: initCustomPlot()
    //         }

    //     }
    //     Component.onCompleted: {
    //         root.forceActiveFocus()
    //     }
    // }

    // Button {
    //         id: button2
    //         anchors.bottom: parent.bottom
    //         anchors.left: parent.left
    //         text: qsTr("Go to Page 1");
    //         onClicked: stackView.push("Page1.qml")
    // }
    // Button {
    //         id: button3
    //         anchors.bottom: parent.bottom
    //         anchors.horizontalCenter: parent.horizontalCenter
    //         text: qsTr("Go to Page 3");
    //         onClicked: stackView.push("Page3.qml")
    // }
    Switch {
        id: startCanvas
        text: qsTr("Start timer")
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        onToggled: {
            lineChart.startTimer(startCanvas.checked);
        }
    }
}
