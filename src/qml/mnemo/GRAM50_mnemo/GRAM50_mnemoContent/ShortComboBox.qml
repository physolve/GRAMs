import QtQuick
import QtQuick.Controls

ComboBox{
    id: cmbx
    width: 52
    // implicitWidth: 48
    height: 25
    // required property var units
    // model: units
    delegate: ItemDelegate {
        id: delegate
        height: 25
        width: cmbx.width - 8
        padding: 0
        leftPadding: 5
        required property var model
        required property int index
        contentItem: Text {
           text: delegate.model[cmbx.textRole]
           color: "black"
           font.pixelSize: 14
           elide: Text.ElideRight
           verticalAlignment: Text.AlignVCenter
       }
        highlighted: cmbx.highlightedIndex === index
    }
    contentItem: Text {
        text: cmbx.displayText
        // width: 35
        height: 25

        font.pixelSize: 14
        elide: Text.ElideRight
        anchors.left:cmbx.left
        anchors.leftMargin: 9
        verticalAlignment: Text.AlignVCenter
        // anchors.verticalCenter: cmbx.verticalCenter
        anchors.horizontalCenter: cmbx.horizontalCenter
    }
    background: Rectangle {
        implicitWidth: 50
        height: 25
        border.color: cmbx.down ? "#D7211C" : "#21be2b"
        border.width: cmbx.visualFocus ? 10 : 1
        radius: 5
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }
    indicator: Rectangle{
        width: 12
        height: 12
        x: cmbx.width - width - 5
        y: (cmbx.availableHeight - height) / 2
        radius: cmbx.down ? 0 : 5
        border.color: cmbx.down ? "#D7211C" : "#17a81a"
    }
    popup: Popup {
        y: cmbx.height - 1
        x: 3
        width: cmbx.width - 8
        // height: 50
        implicitHeight: contentItem.implicitHeight + 2
        padding: 1
        contentItem: ListView {
            clip: true
            interactive: false
            spacing: 1
            implicitHeight: contentHeight
            model: cmbx.popup.visible ? cmbx.delegateModel : null
            currentIndex: cmbx.highlightedIndex

            // ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            width: cmbx.width - 8
            border.color: "#D7211C"
            color: "white"
            radius: 2
        }
    }
}
