import QtQuick
import CustomPlot
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0
import Grams.dataSourceSingleton 1.0

Window {
    // width: 700
    color: "#2B2B2B"
    Item {
        id: chartItem
        anchors.fill: parent
        property int chartIndex: 0
        CustomPlotItem {
            id: customPlotPressure
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            //width: parent.width
            height: 400
            Component.onCompleted: {
                // get index from getFilterPlotPtr
                chartItem.chartIndex = Grams.getFilterPlotPtr(getCustomPlot())
            }
        }
        Button{
            anchors.left: parent.left; anchors.top: customPlotPressure.bottom
            anchors.topMargin: 10
            text: "Обновить фильтр"
            onClicked:{
                DataSource.updateFilter(chartItem.chartIndex)
            }
        }
    }
}