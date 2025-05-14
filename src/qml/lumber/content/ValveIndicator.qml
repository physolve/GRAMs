
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 2.12
Item {
    id: valve
    width: 30
    height: 60
    anchors.verticalCenter: parent.verticalCenter
    property bool state: false
    Button{
        id: indicator
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        checkable: true
        background: Rectangle{
            color: indicator.checked ? "#56BF66" : "#79A6A4"
            border.color: "#56BF66"
            radius: 10
        }
        // checked: state
        // enabled: false
        
    }
}