import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "measure"
Item{
    // property var menuList: ["Параметры", "Выбор камеры"]
    function setParametersMenu() {
        barMainRepeater.model = ["Параметры"]
        layoutMain.currentIndex = 0
    }
    function setChamberPickerMenu() {
        barMainRepeater.model = ["Выбор камеры"]
        layoutMain.currentIndex = 1
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
            model: ["Параметры"] // "График А", "Натекание", "Измерение", 
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
    } 
}