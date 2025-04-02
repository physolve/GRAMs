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
            childWindow.width = 525
            childWindow.height = 600
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

    ChartWindow{
        id: childWindow
        title: "Chart"
        onClosing: {
            holderButton.visible = true
            detachWindow(false)
        }
        Component.onCompleted: {
            detachWindow(true)
            holderButton.visible = false
        }
    }

    ChartFilter{
        id: filterWindow
        onClosing: {
            filterWindow.flags = Qt.FramelessWindowHint
            filterWindow.hide()
            // containerSettings.window = filterWindow
        }
        Component.onCompleted: {
            filterWindow.flags = Qt.Window
            filterWindow.show()
            filterWindow.width = 525
            filterWindow.height = 600
        }
    }
    ChartFilter{
        id: filterWindow1
        onClosing: {
            filterWindow1.flags = Qt.FramelessWindowHint
            filterWindow1.hide()
            // containerSettings.window = filterWindow
        }
        Component.onCompleted: {
            filterWindow1.flags = Qt.Window
            filterWindow1.show()
            filterWindow1.width = 525
            filterWindow1.height = 600
        }
    }

    ChartFilter{
        id: filterWindow2
        onClosing: {
            filterWindow2.flags = Qt.FramelessWindowHint
            filterWindow2.hide()
            // containerSettings.window = filterWindow
        }
        Component.onCompleted: {
            filterWindow2.flags = Qt.Window
            filterWindow2.show()
            filterWindow2.width = 525
            filterWindow2.height = 600
        }
    }

    ChartFilter{
        id: filterWindow3
        onClosing: {
            filterWindow3.flags = Qt.FramelessWindowHint
            filterWindow3.hide()
            // containerSettings.window = filterWindow
        }
        Component.onCompleted: {
            filterWindow3.flags = Qt.Window
            filterWindow3.show()
            filterWindow3.width = 525
            filterWindow3.height = 600
        }
    }
}