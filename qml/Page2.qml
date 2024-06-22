import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import Style
// Page with example Action, Buttons, Dialog, page loading, State change, Animation etc.
Page {
    id: page2
    title: qsTr("Page 2")
    Switch {
        id: kalmanStart
        anchors.top: parent.top
        anchors.left: parent.left
        text: qsTr("Start Filter View")
        font.pointSize: 14
        onToggled: dataSource.turnOnFilterTimer(checked)
    }
        
    BuffChart{
        id: filterPlot
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 60
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

    KalmanTable{
        anchors.top: parent.top
        anchors.left: filterPlot.right
        anchors.leftMargin: 30
        anchors.topMargin: 60
        matrixList: filterView ? filterView.uiA : null
        dimension: 3
    }

    //THIS TableView works on new Qt (>6.5)

    // TableView {
    //     id: tableView
    //     anchors.fill: parent
    //     clip: true

    //     model: TableModel {
    //         TableModelColumn { display: "name" }
    //         rows: [ { "name": "Harry" }, { "name": "Hedwig" } ]
    //     }

    //     selectionModel: ItemSelectionModel {}

    //     delegate: Rectangle {
    //         implicitWidth: 100
    //         implicitHeight: 50

    //         Text {
    //             anchors.centerIn: parent
    //             text: display
    //         }

    //         TableView.editDelegate: TextField {
    //             anchors.fill: parent
    //             text: display
    //             horizontalAlignment: TextInput.AlignHCenter
    //             verticalAlignment: TextInput.AlignVCenter
    //             Component.onCompleted: selectAll()

    //             TableView.onCommit: {
    //                 display = text
    //                 // 'display = text' is short-hand for:
    //                 // let index = TableView.view.index(row, column)
    //                 // TableView.view.model.setData(index, text, Qt.DisplayRole)
    //             }
    //         }
    //     }
    // }

    // TableView.editDelegate
}
