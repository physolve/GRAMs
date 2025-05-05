import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 2.12

Item {
    id: rootItem
    Component.onCompleted: {
        containerSettings.window = leakageAdjustWindow
    }
    function detachWindowSupply(state){ // sender object as arg?
        if(state) {
            containerSettings.window = placeholder
            supplyAdjustWindow.flags = Qt.Window
            supplyAdjustWindow.show()
            supplyAdjustWindow.width = 600
            supplyAdjustWindow.height = 800
        }
        else {
            supplyAdjustWindow.flags = Qt.FramelessWindowHint
            supplyAdjustWindow.hide()
            containerSettings.window = supplyAdjustWindow
        }
    }
    function detachWindowLeakage(state){
        if(state) {
            containerSettings.window = placeholder
            leakageAdjustWindow.flags = Qt.Window
            leakageAdjustWindow.show()
            leakageAdjustWindow.width = 600
            leakageAdjustWindow.height = 800
        }
        else {
            leakageAdjustWindow.flags = Qt.FramelessWindowHint
            leakageAdjustWindow.hide()
            containerSettings.window = leakageAdjustWindow
        }
    }
    ColumnLayout{
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
        Row{
            Button{
                id: holderButtonSupply
                implicitHeight: 40
                implicitWidth: 140
                Layout.alignment: Qt.AlignHCenter
                text: "Detach supply"
                onClicked:{
                    detachWindowSupply(true)
                    holderButtonSupply.visible = false
                }
            }
            Button{
                id: holderButtonLeakage
                implicitHeight: 40
                implicitWidth: 140
                Layout.alignment: Qt.AlignHCenter
                text: "Detach supply"
                onClicked:{
                    detachWindowLeakage(true)
                    holderButtonLeakage.visible = false
                }
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
        id: supplyAdjustWindow
        title: "Настройка подачи газа "
        onClosing: {
            holderButtonSupply.visible = true
            detachWindowSupply(false)
        }
        // Component.onCompleted: {
        //     detachWindowSupply(true)
        //     holderButtonSupply.visible = false
        // }
    }

    LeakageAdjust{
        id: leakageAdjustWindow
        title: "Настройка натекания газа "
        onClosing: {
            holderButtonLeakage.visible = true
            detachWindowLeakage(false)
        }
        // Component.onCompleted: {
        //     detachWindowLeakage(true)
        //     holderButtonLeakage.visible = false
        // }
    }
}