import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 2.12
Item {
    id: rootItem
    Component.onCompleted: {
        containerSettings.window = childWindow
    }
    function detachWindow(state){
        if(state) {
            containerSettings.window = placeholder
            childWindow.flags = Qt.Window
            childWindow.show()
        }
        else {
            childWindow.flags = Qt.FramelessWindowHint
            childWindow.hide()
            containerSettings.window = childWindow
        }
    }
    ColumnLayout{
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
        Button{
            id: holderButton
            implicitHeight: 40
            implicitWidth: 140
            Layout.alignment: Qt.AlignHCenter
            text: "test detach"
            onClicked:{
                detachWindow(true)
                holderButton.visible = false
            }
        }
        WindowContainer {
            Layout.fillHeight: true
            Layout.fillWidth: true
            id: containerSettings
        }
    }
    Window {
        id: placeholder
        color: "transparent"
        flags: Qt.FramelessWindowHint
    }
    
    // connectionWindow page?

    Window {
        id: childWindow

        color: "#2B2B2B"
        ScrollView {
            id: connectionsRoot
            ScrollBar.horizontal.interactive: true
            ScrollBar.vertical.interactive: true
            anchors.fill: parent
            //model view based on keys from Initialize
                //four groups: stuff, controllers, quartiles, security
            // from initSource
            // layout with json information
            SettingsConnections{
                width: childWindow.width
            }
        }
        onClosing: {
            holderButton.visible = true
            detachWindow(false)
        }
    }

}