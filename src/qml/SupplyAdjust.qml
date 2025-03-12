import QtQuick
import LightPlot
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Grams.addRemoveQuartileSingleton 1.0

Window {
    id: root
    color: "#2B2B2B"
    Item {
        id: chartItem
        anchors.fill: parent
        property int chartIndex: 0
        LightPlotItem {
            id: supplyPressureHigh
            anchors.top: parent.top
            width: parent.width
            height: 340
            Component.onCompleted: {
                // get index from getFilterPlotPtr
                chartItem.chartIndex = AddRemoveQuar.getLightPlotPtr(getLightPlot())
                root.title += "High " + chartItem.chartIndex + " / "
            }
        }
        LightPlotItem {
            id: supplyPressureLow
            anchors.top: supplyPressureHigh.bottom
            width: parent.width
            anchors.topMargin: 10
            //width: parent.width
            height: 340
            Component.onCompleted: {
                // get index from getFilterPlotPtr
                chartItem.chartIndex = AddRemoveQuar.getLightPlotPtr(getLightPlot())
                root.title += "Low " + chartItem.chartIndex + " / "
            }
        }
        RowLayout{
            anchors.top: supplyPressureLow.bottom
            width: parent.width
            anchors.topMargin: 10
            spacing: 10
            Button{
                text: "Начать подачу газа"
                onClicked:{
                    if(gasPortChoose.currentIndex == 0){
                        gasPortNotChosen.open()
                        return
                    }
                    const parameters = {
                        supplyPort: gasPortChoose.currentIndex-1,
                        turn: controlTurn.value,
                        portPressure: portPressureCtrl.text
                    }
                    AddRemoveQuar.setSupplyAdjustParameters(parameters)
                    AddRemoveQuar.startSupplyMeasure()
                }
            }
            ComboBox{
                id: gasPortChoose
                model: ["Порты справа-налево...", "Низкое давление H2", "Высокое давление H2", "Инертный газ"]
                Layout.preferredWidth: 200
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
            TextField{
                id: portPressureCtrl
                placeholderText: "Давление источник"
            }
        }
    }
    MessageDialog {
        id: gasPortNotChosen
        buttons: MessageDialog.Ok
        text: "Выбери газовый порт"
    }
}