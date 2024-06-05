import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import CustomPlot
import Style

// Page with some external items defined in separate modules placed in a GridLayout
Page {
    id: page3
    title: qsTr("Page 3")
    Label {
        id: myLabel
        text: qsTr("Page Three")
    }
    TableView {
        id: tableView
        anchors.top: parent.top
        anchors.left: parent.left
        width: 120
        height: 300
        model: _myModel
        delegate: Text {
            id: textLabel
            color: "white"
            width: 100
            padding: 12
            text: ct + ", " + cv
        }
    }

    GridView{
        id: settingViewer
        anchors.top: tableView.bottom
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: parent.width
        flow: GridView.FlowLeftToRight
        anchors.margins: 4
        //cellWidth: 800
        //cellHeight: 300
        //Layout.fillWidth: true
        //Layout.fillHeight: true
        clip: true
        interactive: false
        model: 10
        delegate: Dial {
            id: volumeDial
            from: 0
            value: 42
            to: 100
            stepSize: 1
            //anchors.bottom: parent.bottom
            //anchors.horizontalCenter: parent.horizontalCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.minimumWidth: 64
            Layout.minimumHeight: 64
            Layout.preferredWidth: 128
            Layout.preferredHeight: 128
            Layout.maximumWidth: 128
            Layout.maximumHeight: 128
            Layout.fillHeight: true
            Label {
                text: volumeDial.value.toFixed(0)
                color: "white"
                font.pixelSize: Qt.application.font.pixelSize * 3
                anchors.centerIn: parent
            }
        }
    }
    Component.onCompleted: {
        _myModel.startTestTimer(true);
        console.log("Test Timer started (page loaded)");
    }
}
