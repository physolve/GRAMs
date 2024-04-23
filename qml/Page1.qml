import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Style
import CustomPlot
import "mnemo"
import "mnemo/_GRAM350"
import "mnemo/_GRAM50"
Page{
    id: page1
    title: qsTr("Page 1")
    RowLayout{
        Rectangle{
            id: schemeMnemo
            width: 1350
            height: 900
            color: "transparent" 
            GRAM50Mnemo {
            }
        }
        Chart{
            id: customPlot
            width: 450
            height: 350
            sensorsList: ["DD311"]
        }
    }
}
