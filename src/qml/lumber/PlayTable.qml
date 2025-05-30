import QtQuick
// import TableModel
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

TableView {
    id: tableView
    columnSpacing: 1
    rowSpacing: 1
    boundsBehavior: Flickable.StopAtBounds
    property var columnWidths: [40, 210, 170, 50, 100, 100, 100]
    columnWidthProvider: function (column) { return columnWidths[column] }
    model: TableModel {
        TableModelColumn { display: "runCnt" }
        TableModelColumn { display: "name" }
        TableModelColumn { display: "passed" }
        TableModelColumn { display: "from" }
        TableModelColumn { display: "chargeFrom" }
        TableModelColumn { display: "to" }
        // Each row is one type of fruit that can be ordered
        rows: [
        ]
    }
    Component.onCompleted: {
        model.appendRow({
            runCnt: 0,
            name: 1,
            passed: 2,
            from: 3,
            chargeFrom: 4,
            to: 5
        })
        }

    delegate: DelegateChooser {
        DelegateChoice {
            delegate: TextField {
                text: model.display
                readOnly: true
                selectByMouse: true
                font.pointSize: 10
                // implicitWidth: 76
                horizontalAlignment: TextInput.AlignHCenter
            }
        }
    }
}

