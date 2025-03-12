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
            childWindow.width = 600
            childWindow.height = 800
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
            id: containerSettings
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
    Window {
        id: placeholder
        color: "transparent"
        flags: Qt.FramelessWindowHint
    }
    
    // connectionWindow page?

    SupplyAdjust{
        id: childWindow
        title: "Настройка подачи газа "
        onClosing: {
            holderButton.visible = true
            detachWindow(false)
        }
        Component.onCompleted: {
            detachWindow(true)
            holderButton.visible = false
        }
    }
}