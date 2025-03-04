import QtQuick
import CustomPlot
import Grams.backendSourceSingleton 1.0

Window {
    id: childWindow
    // width: 700
    color: "#2B2B2B"
    Item {
        anchors.fill: parent
        CustomPlotItem {
            id: customPlotPressure
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            width: parent.width;  height: 400
            Component.onCompleted: {
                Grams.getCustomPlotPtr(getCustomPlot())
            }
        }
    }
}