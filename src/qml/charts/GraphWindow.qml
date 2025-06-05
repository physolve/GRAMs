import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import BasePlot
import Grams.backendSourceSingleton 1.0

Item{
    id: root
    Control{
        id: mainChart
        x: 5
        y: 5
        topPadding: 0
        topInset: -2
        leftInset: -2
        rightInset: -6
        bottomInset: -6
        width: parent.width - 10
        height: 360
        contentItem: Grams.mainPlot
        background: Rectangle {
            color:"transparent"; border.color: "#257D97"; border.width: 2; radius: 5
        }
    }

    TabBar {
        id: barCharts
        width: parent.width
        anchors.topMargin: 5
        anchors.top: mainChart.bottom
        anchors.left: parent.left
        //anchors.right: parent.right
        //height: 100
        Repeater{
            id: barChartsRepeater
            model: Grams.chartNames//["График 1", "График 2"] // "График А", "Натекание", "Измерение", 
            TabButton{
                text: modelData
                width: Math.max(120, barCharts.width/barChartsRepeater.count) // /4
                font.pointSize: 12
            }
        }
    }
    StackLayout {
        id: layoutMain
        anchors.topMargin: 5
        anchors.rightMargin: 5
        anchors.bottomMargin: 80
        anchors.top: barCharts.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: barCharts.currentIndex
        Repeater{
            id: chartsRepeater
            model: Grams.graphs
            delegate: Control{
                id: chartView
                x: 5
                y: 5
                topInset: -6
                leftInset: -6
                rightInset: -6
                bottomInset: -6
                width: parent.width - 15
                height: 320
                contentItem: modelData
                background: Rectangle {
                    color:"transparent"; border.color: "#257D97"; border.width: 2; radius: 5
                }
            }
        }
    }
}
    // WindowContainer {
    //             id: myChartWindow
    //             required property int index
    //             required property Item modelData
    //             property bool attached: true
    //             z: 0
    //             // signal detach(name: int)
    //             window: Window {
    //                 id: childWindow
    //                 color:"#2B2B2B"
    //                 flags: Qt.Dialog
    //                 transientParent: null
    //                 Control{
    //                     id: chartView
    //                     x: 5
    //                     y: 5
    //                     topInset: -6
    //                     leftInset: -6
    //                     rightInset: -6
    //                     bottomInset: -6
    //                     width: parent.width - 10
    //                     height: myChartWindow.attached ? 350 : parent.height - 10
    //                     contentItem: modelData
    //                     background: Rectangle {
    //                         color:"transparent"; border.color: "#257D97"; border.width: 2; radius: 5
    //                     }
    //                 }
    //                 Button{
    //                     x: 5
    //                     y: chartView.height+5
    //                     height: 40
    //                     width: 100
    //                     icon.source: "qrc:/shareSVG.svg"
    //                     visible: myChartWindow.attached
    //                     onClicked: {
    //                         var myChart = chartWindow.createObject(root, {win: myChartWindow.window, index})
    //                         myChartWindow.window = placeholder.createObject(root)
    //                         myChart.flags = Qt.Dialog
    //                         myChart.show()
    //                         myChart.width = 525
    //                         myChart.height = 425
    //                         myChartWindow.attached = false
    //                     }
    //                 }
    //             }

    //         }
    //     }


    // Component{
    //     id: chartWindow
    //     Window{
    //         required property var win
    //         required property int index
    //         height: 600
    //         width: 600
    //         visible: false
    //         color: "#2B2B2B" 
    //         WindowContainer {
    //             id: tempWin
    //             anchors.fill: parent
    //             window: win
    //         }
    //         onClosing:{
    //             tempWin.window = chartsRepeater.itemAt(index).window
    //             chartsRepeater.itemAt(index).window = win
    //             chartsRepeater.itemAt(index).attached = true
    //         }
    //     }
    //     onDestroying: {
    //         tempWin.window = chartsRepeater.itemAt(index).window
    //         chartsRepeater.itemAt(index).window = win
    //         chartsRepeater.itemAt(index).attached = true
    //     }
    
    // }
    // Component{
    //     id: placeholder
    //     Window {
    //         color: "#2B2B2B"
    //         // transientParent: null
    //     }
    // }




        // GridView {
    //     id: view
    //     width: parent.width
    //     height: parent.height/3
    //     flow: GridView.FlowTopToBottom
    //     cellWidth: 250; cellHeight: 160
    //     model: Grams.graphs
    // }

    
        // ColumnLayout{
    //     // anchors.fill: parent
    //     anchors.top: barCharts.bottom
    //     anchors.left: parent.left
    //     anchors.right: parent.right
    //     anchors.bottom: parent.bottom
    //     anchors.margins: 10
    //     spacing: 5
    //     Button{
    //         id: holderButton
    //         implicitHeight: 40
    //         implicitWidth: 140
    //         Layout.alignment: Qt.AlignHCenter
    //         text: "test detach"
    //         onClicked:{
    //             // detachWindow(true)
    //             // holderButton.visible = false
    //         }
    //     }
    //     WindowContainer {
    //         id: containerSettings
    //         Layout.fillHeight: true
    //         Layout.fillWidth: true
    //         window: Window {
    //             Control{
    //                 anchors.fill: parent
    //                 contentItem: Grams.graphs[0]
    //             }
    //         }
    //     }
    // }

    
    //  chartWindow
    //         function chartWindowDetach(name) {
    //             console.log(`Detach ${name}`)
    //         }
    //     
    
    
    // ListView{
    //     id: view
    //     width: parent.width
    //     height: parent.height/3
    //     model: Grams.graphs
    //     delegate: WindowContainer {
    //         Layout.fillHeight: true
    //         Layout.fillWidth: true
    //         window: Window {
    //             Control{
    //                 anchors.fill: parent
    //                 contentItem: modelData
    //             }
    //         }
    //     }
    // }

    // Component{
    //     id: chartWindow
    //     Window {
    //         id: childWindow
    //         // color:"transparent"
    //         required property int index
    //         required property Item testItem
    //         Button{
    //             x: parent.width/2 - 50
    //             y: 25
    //             height: 40
    //             width: 100
    //             text: "Новое окно"
    //             onClicked: chartsRepeater.chartWindowDetach(index)
    //         }
    //         Control{
    //             // x: 5
    //             // y: 70
    //             padding: 10
    //             width: parent.width - 10
    //             height: Math.min(350, parent.height - 10)

    //             contentItem: Grams.graphs[index]
    //             background: Rectangle {
    //                 color:"transparent"; border.color: "#464646";
    //             }
    //         }
    //     }
    // }

            // Item {
            //     Button{
            //         x: parent.width/2 - 50
            //         y: 25
            //         height: 40
            //         width: 100
            //         text: "Новое окно"
            //         onClicked: {
            //             var testItem = chartView.contentItem
            //             chartView.contentItem = null
            //             var myChart = chartWindow.createObject(root,{index,testItem})
            //             myChart.flags = Qt.Window
            //             myChart.show()
            //             myChart.width = 525
            //             myChart.height = 600
            //         }
            //     }
            //     Control{
            //         id: chartView
            //         x: 5
            //         y: 70
            //         padding: 10
            //         width: parent.width - 10
            //         height: Math.min(350, parent.height - 10)
            //         contentItem: modelData
            //         background: Rectangle {
            //             color:"transparent"; border.color: "#464646";
            //         }
            //     }
            // }
        
    // Component{
    //     id: chartWindow
    //     WindowContainer {
    //         id: myChartWindow
    //         required property int index
    //         required property Item modelData
    //         // signal detach(name: int)
    //         window: Window {
    //             id: childWindow
    //             color:"transparent"
    //             Button{
    //                 x: parent.width/2 - 50
    //                 y: 25
    //                 height: 40
    //                 width: 100
    //                 text: "Новое окно"
    //                 onClicked: chartsRepeater.chartWindowDetach(index)
    //             }
    //             Control{
    //                 x: 5
    //                 y: 70
    //                 padding: 10
    //                 width: parent.width - 10
    //                 height: Math.min(350, parent.height - 10)

    //                 contentItem: modelData
    //                 background: Rectangle {
    //                     color:"transparent"; border.color: "#464646";
    //                 }
    //             }
    //         }
    //     }
    // }

    // Component {
    //     id: mainChart
    //     BasePlot{

    //     }
    //     Row {
    //         spacing: 10

    //         Button {
    //             text: "Push"
    //             onClicked: stack.push(mainChart)
    //         }
    //         Button {
    //             text: "Pop"
    //             enabled: stack.depth > 1
    //             onClicked: stack.pop()

    //         }
    //         Text {
    //             text: stack.depth
    //         }
    //     }
    // }

    // WindowContainer {
    //     window: Window {
    //         color:"transparent"
    //         signal detach(name: int)
    //         Button{
    //             x: parent.width/2 - 50
    //             y: 25
    //             height: 40
    //             width: 100
    //             text: "Новое окно"
    //             onClicked: detach(index)
    //         }
    //         Control{
    //             x: 5
    //             y: 70
    //             padding: 10
    //             width: parent.width - 10
    //             height: Math.min(350, parent.height - 10)

    //             contentItem: modelData
    //             background: Rectangle {
    //                 color:"transparent"; border.color: "#464646";
    //             }
    //         }
    //     }
    // }