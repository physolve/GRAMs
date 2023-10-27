import QtQuick
import QtQuick.Controls
import QtQml.Models
import QtQuick.Layouts
import CustomPlot
import Style
// Page with example Action, Buttons, Dialog, page loading, State change, Animation etc.
Page {
    id: page2
    title: qsTr("Page 2")

    
    GridView{
        id: listviewer
        anchors.fill: parent
        flow: GridView.FlowLeftToRight
        model: _myModel
        anchors.margins: 4
        cellWidth: 800
        cellHeight: 300
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        interactive: false
        delegate: Item {
            width: 800
            height: 300
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
                target: listviewer.model    // EDIT: I drew the wrong conclusions here, see text below!
                function onDataChanged() {
                    customPlot.backendData(model.x, model.y)
                    //console.log("DataChanged received")
                }
            }
        }
    }

    // ColumnLayout {
    //     anchors.fill: parent
    //     ListView {
    //         id: dataView
    //         Layout.fillWidth: true
    //         Layout.fillHeight: true
    //         model: _myModel
    //         clip: true
    //         interactive: false
    //         delegate: Item {
    //             width: 400
    //             height: 150
    //             CustomPlotItem {
    //                 id: customPlot
    //                 width: parent.width;  height: parent.height // resize
    //                 Component.onCompleted: initCustomPlot(model.index)
    //                 Component.onDestruction: testJSString(model.index)
    //                 function testJSString(num) {
    //                     var text = "I have been destroyed_ %1"
    //                     console.log(text.arg(num))
    //                 }
    //             }
    //             Connections {
    //                 target: dataView.model    // EDIT: I drew the wrong conclusions here, see text below!
    //                 function onDataChanged() {
    //                     customPlot.backendData(model.x, model.y)
    //                     //console.log("DataChanged received")
    //                 }
    //             }
    //         }
    //         focus: true
    //         orientation: ListView.Vertical
    //         ScrollBar.vertical: ScrollBar {
    //         active: true
    //         }
    //     }
    // }

    // GridView{
    //     id: dataViewGrid
    //     anchors.fill: parent
    //     flow: GridView.FlowLeftToRight
    //     model: _myModel
    //     anchors.margins: 4
    //     //cellWidth: 800
    //     //cellHeight: 300
    //     //Layout.fillWidth: true
    //     //Layout.fillHeight: true
    //     clip: true
    //     interactive: false
    //     delegate: 
    // }
    // Rectangle {
    //     id: rect1
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
    //         text: qsTr("Page Two")
    //     }
    // }
    // SplitView {
    //     id: splitView
    //     anchors.fill: parent
    //     orientation: Qt.Vertical
    //     Rectangle {
    //         implicitWidth: 200
    //         SplitView.maximumHeight: 400
    //         color: "lightblue"
    //         ListView {
    //             id: view
    //             anchors.fill: parent
    //             model: dataModel
    //             delegate: Text {
    //                 text: name + " " + value
    //             }
    //         }
    //     }
    //     Rectangle {
    //         id: centerItem
    //         SplitView.minimumHeight: 50
    //         SplitView.fillWidth: true
    //         color: "lightgray"
    //         Label {
    //             text: "View 2"
    //             anchors.centerIn: parent
    //         }
    //     }
    //     Rectangle {
    //         implicitHeight: 200
    //         color: "lightgreen"
    //         LineChart {
    //                 id: lineChart
    //                 width: parent.width/2;  
    //                 //height: parent.height // resize

    //                 title:  'Pendulum Position versus Time'
    //                 yLabel: 'position (degrees)'
    //                 xLabel: 'time (s)'
    //                 color:  'red'
    //         }
    //     }
    //     Rectangle {
    //         id: storeData
    //         width: parent.width 
    //         SplitView.maximumHeight: 100
    //         Button {
    //                 id: testJSONbtn
    //                 anchors.top: storeData.bottom
    //                 text: qsTr("test JSON");
    //                 onClicked: {
    //                     var num = Math.round(Math.random() * 10)
    //                     dataModel.append({ "name": "test" + num, "value": num })
    //                 }
    //         }
    //     }
        
    // }
    
    // Switch {
    //     id: startCanvas
    //     text: qsTr("Start timer")
    //     anchors.bottom: parent.bottom
    //     anchors.right: parent.right
    //     onToggled: {
    //         lineChart.startTimer(startCanvas.checked);
    //     }
    // }




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
    
}
