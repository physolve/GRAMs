import QtQuick
import QtQuick.Controls

RoundButton {
    id: valve
    width: 34
    height: 34
    checkable: true
    property bool doubleClickState: false
    background: Rectangle {
        anchors.fill: parent
        implicitWidth: 34
        implicitHeight: 34
        radius: 17
        opacity: enabled ? 1 : 0.3
        color: valve.down ? "#d4d582" : valve.checked ? "#49e92c" : "#aacba4"
        border.width: 2
        border.color: valve.down ? "gray" : valve.checked ? "#507750" : "#8a938f"
        Rectangle {
           id: rectangle
           x: 5
           y: 5
           width: 24
           height: 24
           radius: 12
           border.color: valve.checked ? "#507750" : "#9a9a9a"
           anchors.verticalCenter: parent.verticalCenter
           anchors.verticalCenterOffset: 0
           rotation: valve.checked ? 90 : -90

           gradient: Gradient {
               orientation: Gradient.Vertical
               GradientStop {
                   id: gradientStop
                   position: 0
                   color: "#8c8c8c"
               }

               GradientStop {
                   id: gradientStop1
                   position: 1
                   color: "#f8f0f0"
               }
           }
           anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    onDoubleClicked:{
        if(!doubleClickState){
            valve.text = "+"
            doubleClickState = true
        }
        else {
            valve.text = ""
            doubleClickState = false
        }
    }
}
