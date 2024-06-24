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
        height: 540
        // sensorsList_first: ["DD311", "DD312", "DD331", "DD332", "DD334"]
        // sensorsList_second: ["DT341", "DT314"]
        testView: {
            "FilteredVoltage": {
                "sensors" : ["voltage_ch0"],
                "first_y" : "Напряжение, В"
            }
        }
        typeView: "view"
    }
    BuffChart{
        id: filterPlotXhatS
        anchors.top: filterPlot.bottom
        anchors.left: parent.left
        anchors.topMargin: 10
        width: 700
        height: 350
        // sensorsList_first: ["DD311", "DD312", "DD331", "DD332", "DD334"]
        // sensorsList_second: ["DT341", "DT314"]
        testView: {
            "FilteredVoltage": {
                "sensors" : ["XhatS_ch0"],
                "first_y" : "X_hat (S)"
            }
        }
        typeView: "xhats"
    }
    BuffChart{
        id: filterPlotXhatT
        anchors.top: filterPlot.bottom
        anchors.left: filterPlotXhatS.right
        anchors.topMargin: 10
        width: 700
        height: 350
        // sensorsList_first: ["DD311", "DD312", "DD331", "DD332", "DD334"]
        // sensorsList_second: ["DT341", "DT314"]
        testView: {
            "FilteredVoltage": {
                "sensors" : ["XhatT_ch0"],
                "first_y" : "X_hat (T)"
            }
        }
        typeView: "xhatt"
    }
    // place for matrix changes
    // table with matrix vectors

    KalmanTable{
        id: kmMatrixA
        anchors.top: parent.top
        anchors.left: filterPlot.right
        anchors.leftMargin: 30
        anchors.topMargin: 60
        matrixList: filterView ? filterView.uiA : null
        dimension: 3
    }
    Label{
        id: kmMatrixAlbl
        text: "Matrix A"
        font.pointSize: 14
        anchors.top: kmMatrixA.bottom
        anchors.left: kmMatrixA.left
    }

    KalmanTable{
        id: kmMatrixC
        anchors.top: parent.top
        anchors.left: kmMatrixA.right
        anchors.leftMargin: 30
        anchors.topMargin: 60
        matrixList: filterView ? filterView.uiC : null
        dimension: 3
    }
    Label{
        id: kmMatrixClbl
        text: "Matrix C"
        font.pointSize: 14
        anchors.top: kmMatrixC.bottom
        anchors.left: kmMatrixC.left
    }

    KalmanTable{
        id: kmMatrixQ
        anchors.top: kmMatrixAlbl.bottom
        anchors.left: filterPlot.right
        anchors.leftMargin: 30
        anchors.topMargin: 30
        matrixList: filterView ? filterView.uiQ : null
        dimension: 3
    }
    Label{
        id: kmMatrixQlbl
        text: "Matrix Q"
        font.pointSize: 14
        anchors.top: kmMatrixQ.bottom
        anchors.left: kmMatrixQ.left
    }

    KalmanTable{
        id: kmMatrixP
        anchors.top: kmMatrixClbl.bottom
        anchors.left: kmMatrixQ.right
        anchors.leftMargin: 30
        anchors.topMargin: 30
        matrixList: filterView ? filterView.uiP : null
        dimension: 3
    }
    Label{
        id: kmMatrixPlbl
        text: "Matrix P"
        font.pointSize: 14
        anchors.top: kmMatrixP.bottom
        anchors.left: kmMatrixP.left
    }

    TextField{
        id: kmMatrixR
        anchors.top: kmMatrixQlbl.bottom
        anchors.left: filterPlot.right
        anchors.leftMargin: 30
        anchors.topMargin: 30
        width: 40
        text : filterView ? filterView.uiR : null
        font.pointSize: 12
        color: 'white'
        selectByMouse: true
        validator: DoubleValidator { bottom: 0.001; top: 10000; decimals: 3}
        // anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: TextInput.AlignHCenter
        onEditingFinished:{
            // modelData = parseFloat(text)
            filterView.uiR = parseFloat(text)
        }
    }
    Label{
        text: "Matrix R"
        font.pointSize: 14
        anchors.top: kmMatrixR.bottom
        anchors.left: kmMatrixR.left
    }

    Button{
        id: saveResult
        anchors.top: kmMatrixPlbl.bottom
        anchors.left: kmMatrixP.left
        anchors.leftMargin: 30
        anchors.topMargin: 30
        text: "Применить"
        font.pixelSize: 12
        onClicked: dataSource.setNewFilter()
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
