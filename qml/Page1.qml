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
                width: 1350
                height: 900
        }
    }
    Component{
        id: gram50mnemo
        GRAM50Mnemo {
                //id: gRAMsMnemoForm
                width: 1350
                height: 900
        }
    }
    property int profileId: 0
    StackLayout {
        id: stackLayout
        anchors.fill: parent
        RowLayout{
            Item {
                id: mnemo
                Loader { 
                    sourceComponent: (profileId == 0 ? gram350mnemo : gram50mnemo) 
                }
            }
            ColumnLayout {
                Layout.preferredWidth: 490
                Layout.fillHeight: true
                ListView {
                    id: dataView
                    model: _myModel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    interactive: false
                    delegate: Item {
                        width: 480
                        height: 180
                        CustomPlotItem {
                            id: customPlot
                            width: parent.width;  height: parent.height // resize
                            Component.onCompleted: initCustomPlot(model.index)
                            Component.onDestruction: testJSString(model.index)
                            function testJSString(num) {
                                var text = "I have been destroyed_ %1"
                                console.log(text.arg(num))
                            }
                        }
                        Connections {
                            target: dataView.model    // EDIT: I drew the wrong conclusions here, see text below!
                            function onDataChanged() {
                                customPlot.backendData(model.x, model.y)
                            }
                        }
                    }
                    focus: true
                    orientation: ListView.Vertical
                    ScrollBar.vertical: ScrollBar {
                    active: true
                    }
                }
            }
        }
    }
    Connections {
        target: _myModel
        onDataChanged: {
            gRAMsMnemoForm.setPressureVal(_myModel.getCurPressureValues()) // it's Working fine
            gRAMsMnemoForm.setTempVal(_myModel.getCurTempValues())
        }
    }

}