import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
//import Qt.labs.settings
import QtQuick.Dialogs
//import Style
import "mnemo"
ApplicationWindow {
    id: main
    width: 1840
    height: 960
    visible: true
    title: qsTr("GRAMs") //: some information
    Material.theme: Material.Dark
    Material.accent: Material.Indigo
    //property string datastore: ""
    property int profileId: 0
    // Settings {
    //     property alias datastore: main.datastore
    // }
    // ListModel {
    //     id: dataModel
    //     ListElement { name: "test1"; value: 1 }
    // }
    Component.onCompleted: {
        main.hide()
        winld.active = true
    }
    Loader {
        id: winld
        active: false
        asynchronous: true //test
        visible: status == Loader.Ready //test
        sourceComponent:  Initialize{
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
       //cfgWindow.saveSetUp()
       //console.log("save setup function")
    }

    header: TabBar {
        id: bar
        width: parent.width
        Repeater{
            model: ["Mnemo", "Data", "Settings"]
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
        Repeater {
            model: ObjectModel
            {
                id: mainPageContainer
            }
        }
    }

    function initializeEnd(){
        // check main.profileId to know which Page1 to load
        // now put usual page 1 with GRAM50 mnemo
        let tabPage1 = page1.createObject(stackLayout)
        mainPageContainer.append(tabPage1)
        let tabPage2 = page2.createObject(stackLayout)
        mainPageContainer.append(tabPage2)
        backend.initializeReading()

    }

}
