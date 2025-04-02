import QtQuick
import LightPlot
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Grams.dataSourceSingleton 1.0
import Grams.reactionQuartileSingleton 1.0

Window {
    id: root
    color: "#2B2B2B"
    Item {
        id: chartItem
        anchors.fill: parent
        property int chartIndex: 0
        LightPlotItem {
            id: leakagePressureHigh
            anchors.top: parent.top
            width: parent.width
            height: 340
            Component.onCompleted: {
                // get index from getFilterPlotPtr
                chartItem.chartIndex = ReactionQuar.getLightPlotPtr(getLightPlot())
                root.title += "High " + chartItem.chartIndex + " / "
            }
        }
        LightPlotItem {
            id: leakagePressureLow
            anchors.top: leakagePressureHigh.bottom
            width: parent.width
            anchors.topMargin: 10
            //width: parent.width
            height: 340
            Component.onCompleted: {
                // get index from getFilterPlotPtr
                chartItem.chartIndex = ReactionQuar.getLightPlotPtr(getLightPlot())
                root.title += "Low " + chartItem.chartIndex + " / "
            }
        }
        RowLayout{
            anchors.top: leakagePressureLow.bottom
            width: parent.width
            anchors.topMargin: 10
            spacing: 10
            Button{
                checkable: true
                text: checked ? "Остановить натекание газа" : "Начать натекание газа"
                onClicked:{
                    if(leakageValveChoose.currentIndex == 0){
                        leakageValveNotChosen.open()
                        checked = false
                        return
                    }
                    const parameters = {
                        leakageValve: leakageValveChoose.currentIndex-1,
                        turn: controlTurn.value
                    }
                    if(checked){
                        ReactionQuar.setReactionAdjustParameters(parameters)
                    }
                    if(DataSource.setLeakageMeasure(checked)){
                        ReactionQuar.startLeakageMeasure(checked)
                    }
                }
            }
            ComboBox{
                id: leakageValveChoose
                model: ["Порты сверху-вниз...", "Медленный", "Средний", "Быстрый"]
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
        }
    }
    MessageDialog {
        id: leakageValveNotChosen
        buttons: MessageDialog.Ok
        text: "Выбери натекатель"
    }
}