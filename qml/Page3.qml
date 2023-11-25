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
    
    // Button {
    //         id: button2
    //         anchors.bottom: parent.bottom
    //         anchors.left: parent.left
    //         text: qsTr("Go to Page 2");
    //         onClicked: stackView.push("Page2.qml")
    // }
    // Button {
    //         id: button1
    //         anchors.bottom: parent.bottom
    //         anchors.right: parent.right
    //         text: qsTr("Go to Page 1");
    //         onClicked: stackView.push("Page1.qml")
    // }
    Component.onCompleted: {
        _myModel.startTestTimer(true);
        console.log("Test Timer started (page loaded)");
    }
}
    // GridLayout {
    //     id: myGrid1
    //     anchors.fill: parent
    //     columns: 2
    //     columnSpacing: 0; rowSpacing: 0
    //     anchors.top: myLabel.bottom
    //     // ListView {
    //     //     id: listviewer
    //     //     model: _myModel
    //     //     interactive: false
    //     //     Layout.fillHeight: true
    //     //     Layout.fillWidth: true
    //     //     delegate: Item {
    //     //         width: 800
    //     //         height: 300

    //     //         // Image {
    //     //         //     id: image
    //     //         //     source: model.flag
    //     //         //     width: 64
    //     //         //     height: 64
    //     //         //     fillMode: Image.PreserveAspectFit

    //     //         //     anchors { left:parent.left }
    //     //         // }

    //     //         // Text {
    //     //         //     text: model.name + "\n" +"population: " + model.population.toFixed(3) + " mill."
    //     //         //     anchors { left:image.right; verticalCenter: image.verticalCenter; leftMargin: 5 }
    //     //         // }
    //     //         // MouseArea {
    //     //         //     anchors.fill: parent
    //     //         //     acceptedButtons: Qt.LeftButton | Qt.RightButton
    //     //         //     onDoubleClicked: {
    //     //         //         if (mouse.button === Qt.LeftButton) {
    //     //         //             _myModel.duplicateData(model.index);
    //     //         //         } else if (mouse.button === Qt.RightButton) {
    //     //         //             _myModel.removeData(model.index);
    //     //         //         }
    //     //         //     }
    //     //         // }

    //     //         CustomPlotItem {
    //     //             id: customPlot
    //     //             width: parent.width;  height: parent.height // resize

    //     //             Component.onCompleted: initCustomPlot()
    //     //         }

    //     //         Connections {
    //     //             target: listviewer.model    // EDIT: I drew the wrong conclusions here, see text below!
    //     //             function onDataChanged() {
    //     //                 customPlot.backendData(model.x, model.y)
    //     //                 //console.log("DataChanged received")
    //     //             }
    //     //         }
    //     //     }
    //     //     ScrollBar.vertical: ScrollBar { }
    //     // }
    //     // test gaude
    //     // Dial {
    //     //     id: volumeDial
    //     //     from: 0
    //     //     value: 42
    //     //     to: 100
    //     //     stepSize: 1

    //     //     Layout.alignment: Qt.AlignHCenter
    //     //     Layout.minimumWidth: 64
    //     //     Layout.minimumHeight: 64
    //     //     Layout.preferredWidth: 128
    //     //     Layout.preferredHeight: 128
    //     //     Layout.maximumWidth: 128
    //     //     Layout.maximumHeight: 128
    //     //     Layout.fillHeight: true
    //     //     Label {
    //     //         text: volumeDial.value.toFixed(0)
    //     //         color: "white"
    //     //         font.pixelSize: Qt.application.font.pixelSize * 3
    //     //         anchors.centerIn: parent
    //     //     }
    //     // }
    //  }
