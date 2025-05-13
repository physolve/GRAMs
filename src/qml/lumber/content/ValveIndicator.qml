
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 2.12
Item {
    id: valve
    width: 40
    height: 40
    property bool state: false
    RoundButton{
        id: indicator
        anchors.centerIn: parent
        width: 40
        height: 40
        checkable: true
        background: Rectangle{
            color: indicator.checked ? "#56BF66" : "#79A6A4"
            border.color: "#56BF66"
            radius: 20
        }
        // checked: state
        // enabled: false
        
    }
}