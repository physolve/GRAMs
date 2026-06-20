import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "measure"
import "charts"
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
    function setGraphWindow(){
        barMainRepeater.model = ["Графики"]
        layoutMain.currentIndex = 6
    }
    function setUserExperimentMenu(){
        barMainRepeater.model = ["Создание эксперимента"]
        layoutMain.currentIndex = 7
    }
    function setValveTestMenu(){
        barMainRepeater.model = ["Тест клапанов"]
        layoutMain.currentIndex = 8
    }

    // Called from Main.qml when a regime button is clicked in RunTable.
    // Opens the settings page that matches the regime name.
    function openRegimePage(name, regimeIndex) {
        switch (name) {
        case "Вакуум":
            setPlayVacuumMenu()
            break
        case "Тест клапанов":
            setValveTestMenu()
            break
        case "Напуск газа":
            setPlayInletMenu()
            break
        case "Автоматическая подача газа":
            setExpSupplyMenu()
            break
        default:
            // Режим в / Режим г / unknown — open valve test as a fallback editor
            setValveTestMenu()
            break
        }
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
            model: ["Графики"] // "График А", "Натекание", "Измерение", 
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
        currentIndex: 6//barMain.currentIndex
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
        GraphWindow{
            id: graphWindow
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        UserExperiment{
            id: userExperiment
            Layout.fillWidth: true
            Layout.fillHeight: true
            onOpenESupply: setExpSupplyMenu()
        }
        ValveTestSetup {
            id: valveTestSetup
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}