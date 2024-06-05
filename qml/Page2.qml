import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import Style
// Page with example Action, Buttons, Dialog, page loading, State change, Animation etc.
Page {
    id: page2
    title: qsTr("Page 2")
    header: Switch {
        id: startBtn
        text: qsTr("Start Filter View")
        font.pointSize: 14
        onToggled: dataSource.turnOnFilterTimer()
    }
    
    RowLayout{
        anchors.fill: parent
        
        BuffChart{
            id: filterPlot
            width: 1400
            height: 700
            // sensorsList_first: ["DD311", "DD312", "DD331", "DD332", "DD334"]
            // sensorsList_second: ["DT341", "DT314"]
            testView: {
                "FilteredVoltage": {
                    "sensors" : ["voltage_ch0"],
                    "first_y" : "voltage"
                }
            }
        }
        // place for matrix changes
        // table with matrix vectors
        TableView {
            width: 250
            height: 500
            columnSpacing: 1
            rowSpacing: 1
            boundsBehavior: Flickable.StopAtBounds

            model: TableModel {
                TableModelColumn { display: "checked" }
                TableModelColumn { display: "amount" }
                TableModelColumn { display: "fruitType" }
                TableModelColumn { display: "fruitName" }
                TableModelColumn { display: "fruitPrice" }

                // Each row is one type of fruit that can be ordered
                rows: [
                    {
                        // Each property is one cell/column.
                        checked: false,
                        amount: 1,
                        fruitType: "Apple",
                        fruitName: "Granny Smith",
                        fruitPrice: 1.50
                    },
                    {
                        checked: true,
                        amount: 4,
                        fruitType: "Orange",
                        fruitName: "Navel",
                        fruitPrice: 2.50
                    },
                    {
                        checked: false,
                        amount: 1,
                        fruitType: "Banana",
                        fruitName: "Cavendish",
                        fruitPrice: 3.50
                    }
                ]
            }
            delegate: TextInput {
                    text: model.display
                    padding: 12
                    selectByMouse: true

                    onAccepted: model.display = text

                    Rectangle {
                        anchors.fill: parent
                        color: "#efefef"
                        z: -1
                    }
                }
            }
    }

    // TableView.editDelegate
}
