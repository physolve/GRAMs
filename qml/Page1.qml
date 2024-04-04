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
        // Item {
        //     width: 300
        //     height: 150
        //     CustomPlotItem {
        //         id: customPlot
        //         width: parent.width;  height: parent.height // resize
        //         Component.onCompleted: initCustomPlot(1)
        //         Component.onDestruction: testJSString(1)
        //         function testJSString(num) {
        //             var text = "I have been destroyed_ %1"
        //             console.log(text.arg(num))
        //         }
        //     }
        //     Connections {
        //         target: _myModel    // EDIT: I drew the wrong conclusions here, see text below!
        //         function onDataChanged() {
        //             // customPlot.backendData(model.x, model.y)
        //             // console.log("DataChanged received ", model.x, model.y) model is not defined
        //         }
        //     }
        // }     
    }
    
   

    // Connections {
    //     target: _myModel
    //     onDataChanged: {
    //         mnemo.item.setPressureVal(_myModel.getCurPressureValues()) //gRAMsMnemoForm  it's Working fine
    //         mnemo.item.setTempVal(_myModel.getCurTempValues()) //gRAMsMnemoForm 
    //     }
    // }


    // ColumnLayout {
    //     Layout.preferredWidth: 490
    //     Layout.fillHeight: true
    //     ListView {
    //         id: dataView
    //         model: _myModel
    //         Layout.fillWidth: true
    //         Layout.fillHeight: true
    //         clip: true
    //         interactive: false
    //         delegate: Item {
    //             width: 480
    //             height: 180
    //             CustomPlotItem {
    //                 id: customPlot
    //                 width: parent.width;  height: parent.height // resize
    //                 Component.onCompleted: initCustomPlot(model.index)
    //                 Component.onDestruction: testJSString(model.index)
    //                 function testJSString(num) {
    //                     var text = "I have been destroyed_ %1"
    //                     console.log(text.arg(num))
    //                 }
    //             }
    //             Connections {
    //                 target: dataView.model    // EDIT: I drew the wrong conclusions here, see text below!
    //                 function onDataChanged() {
    //                     customPlot.backendData(model.x, model.y)
    //                 }
    //             }
    //         }
    //         focus: true
    //         orientation: ListView.Vertical
    //         ScrollBar.vertical: ScrollBar {
    //         active: true
    //         }
    //     }
    // }
}
