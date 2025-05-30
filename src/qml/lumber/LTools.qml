import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: lTools
    color: "#464646"
    border.color: "#F2C029"
    anchors.fill: parent
    // reminder to change for other systems
    HorizontalHeaderView {
        id: horizontalHeader
        syncView: playTable
        x: 5
        y: 10
        // implicitHeight: 36
        model: ["№", "Режим", "Начать при условии", "Повтор", "Статус","Время"]
        clip: true
        // delegate: Rectangle {
        //     color: "white"
        //     Text { text: modelData; color: "black" }
        // }
        // movableColumns: false
    }
    ScrollView {
        id: tableScrollExp
        x:5
        y:5
        height: parent.height-10
        width: parent.width-10
        background: Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "#ABDBDD"
            border.width: 2
            radius: 5
        }
        PlayTable{
            id: playTable
            topMargin: horizontalHeader.implicitHeight+5
            leftMargin: 5
            // onClicked: function(row, rowData) { print('onClicked', row, JSON.stringify(rowData)); }
        }
        // contentWidth: children.implicitWidth
        // contentHeight: children.implicitHeight
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        function scrollToBottom(){
            ScrollBar.vertical.position = 1.0 - ScrollBar.vertical.size
        }
        clip: true
    }
    RoundButton{
        x: 5
        y: parent.height-55
        id: expSettingsBtn
        // width: 50
        icon.source: "qrc:/plusSVG.svg"
        icon.color: "#F2C029"
        onClicked: lTools.openESupply()
    }
    
    signal openESupply()
}