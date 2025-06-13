import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Grams.backendSourceSingleton 1.0
import Grams.addRemoveQuartileSingleton 1.0

Window {
    id: root
    color: "#2B2B2B"
    flags: Qt.Dialog
    Item {
        id: chartItem
        anchors.fill: parent
        property int chartIndex: 0
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
            contentItem: AddRemoveQuar.addRemoveGraphs[0]
            background: Rectangle {
                color:"transparent"; border.color: "#257D97"; border.width: 2; radius: 5
            }
        }
        Row{
            x: 5
            y: 400
            width: parent.width - 10
            spacing: 10
            // Button{
            //     checkable: true
            //     text: checked ? "Остановить подачу газа" : "Начать подачу газа"
            //     onClicked:{
            //         if(gasPortChoose.currentIndex == 0){
            //             gasPortNotChosen.open()
            //             checked = false
            //             return
            //         }
            //         // const parameters = {
            //         //     supplyPort: gasPortChoose.currentIndex-1,
            //         //     turn: controlTurn.value,
            //         //     portPressure: portPressureCtrl.text
            //         // }
            //     }
            // }
            Button{
                text: "Подача газа"
                onClicked: {
                    if(gasPortChoose.currentIndex == 0){
                        gasPortNotChosen.open()
                        checked = false
                        return
                    }
                    Grams.testActionHandler()
                }
            }
            ComboBox{
                id: gasPortChoose
                model: ["Порты слева-направо...", "Инертный газ", "Высокое давление H2", "Низкое давление H2"]
                implicitContentWidthPolicy: ComboBox.ContentItemImplicitWidth
                onActivated: {
                    
                }
            }
            Dial{
                id: controlTurn
                to: 10
                stepSize: 0.5
                snapMode: Dial.SnapAlways
                Label{
                    anchors.centerIn: parent
                    text: controlTurn.value.toFixed(1)
                }
            }
        }
    }
    MessageDialog {
        id: gasPortNotChosen
        buttons: MessageDialog.Ok
        text: "Выбери газовый порт"
    }
    Component.onCompleted: {
        AddRemoveQuar.initAddRemoveCharts()
        root.show()
        root.width = 525
        root.height = 725
    }
    onClosing:{
        mainChart.contentItem = null
        AddRemoveQuar.clearAddRemoveCharts()
    }
}