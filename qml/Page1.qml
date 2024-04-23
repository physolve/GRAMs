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
    Component{
        id: gram350mnemo
        GRAMsMnemoForm {
            //id: gRAMsMnemoForm
            width: 1350 //?
            height: 900 //?
        }
    }
    Component{
        id: gram50mnemo
        GRAM50Mnemo {
        }
    }
    Component.onCompleted: {
        console.log(main.profileId)

    }
    RowLayout{
        Rectangle{
            id: schemeMnemo
            width: 1350
            height: 900
            color: "transparent" 
            Loader { 
                id: mnemo
                sourceComponent: (main.profileId == 0 ? gram350mnemo : gram50mnemo)
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
