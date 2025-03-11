import QtQuick
import CustomPlot
import QtQuick.Controls
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
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            //width: parent.width
            height: 400
            Component.onCompleted: {
                // get index from getFilterPlotPtr
                chartItem.chartIndex = AddRemoveQuar.getLightPlotPtr(getLightPlot())
                root.title = "AddRemove High " + chartItem.chartIndex 
            }
        }
        LightPlotItem {
            id: supplyPressureLow
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: supplyPressureHigh.bottom
            anchors.topMargin: 10
            //width: parent.width
            height: 400
            Component.onCompleted: {
                // get index from getFilterPlotPtr
                chartItem.chartIndex = AddRemoveQuar.getLightPlotPtr(getLightPlot())
                root.title = "AddRemove Low " + chartItem.chartIndex 
            }
        }
        Button{
            anchors.left: parent.left; anchors.top: supplyPressureLow.bottom
            anchors.topMargin: 10
            text: "Начать подачу газа"
            onClicked:{

            }
        }
    }
}