import QtQuick 2.15
import QtQuick.Controls 2.15
//import QtQuick.Controls.Material
Rectangle {
    id: sensor
    width: 148
    height: 40
    radius: 14
    color: "#a6bfb3"
    property double value: 0.0012
    Row {
        anchors.fill: parent
        spacing: 2
        anchors.leftMargin: 4
        Rectangle {
            id: display
            width: 80
            height: sensor.height - 4
            radius: 10
            color: "#61736c"
            anchors.verticalCenter: parent.verticalCenter
            Label {
                id: label
                text: Number(sensor.value.toFixed(3)) //toPrecision
                font.pixelSize: 17
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        ComboBox {
            id: root
            width: sensor.width - display.width - 3 - 5
            height: sensor.height
            model: ["Exp", "Bar", "Pa"]
            delegate: ItemDelegate {
               width: root.width + 8
               contentItem: Text {
                   text: root.textRole
                       ? (Array.isArray(root.model) ? modelData[root.textRole] : model[root.textRole])
                       : modelData
                   //color: "#21be2b"
                   renderType: Text.NativeRendering;
                   font.pixelSize: 12
                   elide: Text.ElideRight
                   verticalAlignment: Text.AlignVCenter
                   horizontalAlignment: Text.AlignLeft
               }
               highlighted: root.highlightedIndex === index
            }
            background: Rectangle {
                implicitWidth: root.width - 5
                height: root.height - 8
                border.color: root.down ? "#D7211C" : "#21be2b"
                border.width: root.visualFocus ? 10 : 1
                radius: 8
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            contentItem: Text {
                text: root.displayText
                height: root.height - 8
                width: root.width - 20
                font.pixelSize: 14
                renderType: Text.NativeRendering;
                //color: root.down ? "#D7211C" : "#21be2b"
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                anchors.left: root.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
            }

            indicator: Rectangle{
                id: m_indicator
                width: 12
                height: 12
                x: root.width - width - 5
                y: (root.availableHeight - height) / 2
                radius: root.down ? 0 : 5
                border.color: root.down ? "#D7211C" : "#17a81a"
            }

            popup: Popup {
                x: -0
                y: root.height - 1
                width: root.width
                implicitHeight: contentItem.implicitHeight
                padding: -8

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: root.popup.visible ? root.delegateModel : null
                    currentIndex: root.highlightedIndex
                    interactive: false
                }

                background: Rectangle {
                    border.color: "#21be2b"
                    radius: 2
                }
            }
        }

    }

//            RoundButton {
//                id: button
//                width: sensor.width - display.width
    //                height: sensor.height
    //            }

}
