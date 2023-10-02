import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.settings
import QtQuick.Dialogs
//import Style

ApplicationWindow {
    id: main
    width: 1840
    height: 940
    visible: true
    title: qsTr("GRAMs") //: some information
    Material.theme: Material.Dark
    Material.accent: Material.Indigo
    property string datastore: ""
    Settings {
        property alias datastore: main.datastore
    }
    ListModel {
        id: dataModel
        ListElement { name: "test1"; value: 1 }
    }
    Component.onCompleted: {
        main.hide()
        winld.active = true
    }

    Loader {
        id: winld
        active: false
        asynchronous: true //test
        visible: status == Loader.Ready //test
        sourceComponent:  SetUpGRAMs{
        }
    }

    Component{
        id: page1
        Page1{
            //headerColor: Style.header.color.primary
        }
    }
    Component {
        id: page2
        Page2 {}
    }
    Component {
        id: page3
        Page3 {}
    }

    onClosing: {
       cfgWindow.saveSetUp()
    }

    header: TabBar {
        id: bar
        width: parent.width
        Repeater{
            model: ["First", "Second", "Third", "Fourth", "Fifth"]
            TabButton{
                text: modelData
                width: Math.max(100, bar.width/5)
            }
        }
    }

    StackLayout {
        id: stackLayout
        width: parent.width
        currentIndex: bar.currentIndex
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        // Page1 {
        //     headerColor: Style.header.color.primary
        // }
        Repeater {
            model: ObjectModel
            {
                id: container
            }
        }
    }
}