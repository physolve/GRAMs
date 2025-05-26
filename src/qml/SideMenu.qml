import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "measure"
Item{
    // property var menuList: ["Параметры", "Выбор камеры"]
    function setParametersMenu() {
        barMainRepeater.model = ["Настройки"]
        layoutMain.currentIndex = 0
    }
    function setChamberPickerMenu() {
        barMainRepeater.model = ["Выбор камеры"]
        layoutMain.currentIndex = 1
    }
    function setPlayVacuumMenu() {
        barMainRepeater.model = ["Откачка"]
        layoutMain.currentIndex = 2
    }
    function setPlaySampleMenu(){
        barMainRepeater.model = ["Образец и тигель"]
        layoutMain.currentIndex = 3
    }
    function setPlayInletMenu(){
        barMainRepeater.model = ["Напуск газа"]
        layoutMain.currentIndex = 4
    }
    function setExpSupplyMenu(){
        barMainRepeater.model = ["Автоматическая подача газа"]
        layoutMain.currentIndex = 5
    }
    TabBar {
        id: barMain
        width: parent.width
        anchors.top: parent.top
        anchors.left: parent.left
        //anchors.right: parent.right
        //height: 100
        Repeater{
            id: barMainRepeater
            model: ["Настройки"] // "График А", "Натекание", "Измерение", 
            TabButton{
                text: modelData
                width: Math.max(120, barMain.width) // /4
                font.pointSize: 12
            }
        }
    }

    StackLayout {
        id: layoutMain
        anchors.top: barMain.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: 0//barMain.currentIndex
        // ExpCreator{
        //     id: test1
        //     Layout.fillWidth: true
        //     Layout.fillHeight: true
        // }  
        // ChartCreator{
        //     id: test2
        //     Layout.fillWidth: true
        //     Layout.fillHeight: true
        // }
        // SettingsWindow{
        //     id: test4
        //     Layout.fillWidth: true
        //     Layout.fillHeight: true
        // }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            SettingsConnections{
            }
        }
        ChamberChoose{
            id: chamberChooser
            Layout.fillWidth: true
            // Layout.fillHeight: true
        }
        PVacuum{
            id: pVacuum
            Layout.fillWidth: true
            // Layout.fillHeight: true
        }
        PSample{
            id: pSample
            Layout.fillWidth: true
        }
        PInlet{
            id: pInlet
            Layout.fillWidth: true
        }
        ESupply{
            id: eSupply
            Layout.fillWidth: true
        }
    } 
}