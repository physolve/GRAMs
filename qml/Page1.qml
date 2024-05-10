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
        ColumnLayout{
            Chart{
                id: customPlot
                width: 450
                height: 700
                // sensorsList_first: ["DD311", "DD312", "DD331", "DD332", "DD334"]
                // sensorsList_second: ["DT341", "DT314"]
                testView: {
                    "only_pressure": {
                        "sensors" : ["DD311", "DD312", "DD331", "DD332", "DD334"],
                        "first_y" : "pressure"
                    },
                    "only_temperature":{
                        "sensors" : ["DT341", "DT314"],
                        "first_y" : "temp"
                    } 
                }
            }
        }
    }
}
