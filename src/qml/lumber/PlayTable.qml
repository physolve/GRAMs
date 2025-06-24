import QtQuick
// import TableModel
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import QtQuick.Controls.Material 
import "content"
import "regime"

TableView {
    id: tableView
    columnSpacing: 1
    rowSpacing: 1
    boundsBehavior: Flickable.StopAtBounds
    selectionBehavior: TableView.SelectRows
    selectionMode: TableView.SingleSelection
    property var columnWidths: [80, 270, 80, 110, 100]
    columnWidthProvider: function (column) { return columnWidths[column] }
    model: TableModel {
        TableModelColumn { display: "name" }
        TableModelColumn { display: "passed" }
        TableModelColumn { display: "from" }
        TableModelColumn { display: "time" }
        TableModelColumn { display: "setting" }
        // Each row is one type of fruit that can be ordered
        rows: [
        ]
    }
    selectionModel: ItemSelectionModel {
    }
    // Component.onCompleted: {
    //     model.appendRow({
    //         name: "Вакуум",
    //         passed: 0,
    //         from: 3,
    //         time: "00:00:00",
    //         setting: 0.5
    //     })
    // }
    signal openESupply()
    function openParameters(regime, row){
        console.log(regime, row)
        if(regime == "Цел. в камере"){
            // list of regime var ->
            // just show if closed
            supplyAdjustWindow.createObject(parent)
        }
            // tableView.openESupply()
    }
    delegate: DelegateChooser {
        DelegateChoice {
            column: 0
            delegate: Button {
                required property bool current
                required property bool selected
                text: model.display
                Material.roundedScale: Material.ExtraSmallScale
                highlighted: selected
                MouseArea {
                    anchors.fill: parent
                    onPressAndHold: {
                        tableView.selectionModel.select(tableView.model.index(row, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Current | ItemSelectionModel.Rows);
                    }
                    onReleased:{
                        tableView.selectionModel.clearSelection()
                    }
                    onClicked: openParameters(text, row)
                }
            }
        }
        DelegateChoice {
            column: 1
            delegate:Row{
                required property bool selected
                required property bool current
                property int chosenCase: model.display
                ComboBox {
                    model: ["Темп. камера", "Время"]
                    width: parent.width/2
                    currentIndex: chosenCase
                    font.pointSize: 9
                }
                TextField{
                    font.pointSize: 9
                    validator: DoubleValidator { bottom: 0; top: 1000} //; decimals: 3
                    selectByMouse: true
                    width: parent.width/4
                    horizontalAlignment: TextInput.AlignHCenter
                    placeholderText: "°C"
                }
                TextField{
                    font.pointSize: 9
                    validator: DoubleValidator { bottom: 0; top: 1000} //; decimals: 3
                    selectByMouse: true
                    width: parent.width/4
                    horizontalAlignment: TextInput.AlignHCenter
                    placeholderText: "мин"
                }
            } 
        }
        DelegateChoice {
            column: 2
            delegate: SpinBox {
                required property bool selected
                required property bool current
                id: customSpin
                value: model.display
                onValueModified: model.display = value
                up.indicator:  ScrollArrow {
                    arrowColor: "white"
                    transform: Translate {x: customSpin.width-22 ; y: customSpin.height/2-16}
                }
                down.indicator: ScrollArrow {
                    arrowColor: "white"
                    rotation: 180
                    transform: Translate {x: customSpin.width-22; y: customSpin.height/2-4}
                }
            }
        }
        DelegateChoice {
            column: 3
            delegate: TextField {
                required property bool selected
                required property bool current
                text: model.display
                readOnly: true
                implicitWidth: 90
                font.pointSize: 10
                inputMask: "99:99:99"
                inputMethodHints: Qt.ImhTime
                horizontalAlignment: TextInput.AlignHCenter
                color: selected ? "lightblue" : "white"
            }
        }
        DelegateChoice {
            column: 4
            delegate: CuteProgressBar {
                required property bool selected
                required property bool current
                value: model.display
                leftPadding: 6
                rightPadding: 6
                topPadding: 6
                bottomPadding: 6
                topInset: 6
                leftInset: 6
                rightInset: 6
                bottomInset: 6
            }
        }
        DelegateChoice {
            delegate: TextField {
                required property bool selected
                required property bool current
                text: model.display
                readOnly: true
                selectByMouse: true
                font.pointSize: 10
                // implicitWidth: 76
                horizontalAlignment: TextInput.AlignHCenter
            }
        }
    }
    Component{
        id: supplyAdjustWindow
        SupplyRegime{
        }
    }
}
