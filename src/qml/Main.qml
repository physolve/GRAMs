import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 2.12
import Grams.backendSourceSingleton 1.0

//import "mnemo/GRAM300_mnemo/GRAM300_mnemoContent"
import "mnemo/GRAM50_mnemo/GRAM50_mnemoContent"
import "measure"
import "lumber"

ApplicationWindow {
    id: main
    width: 1900
    height: 1000
    visible: true
    title: qsTr("GRAMs")
    visibility: Window.Maximized
    // Material.theme: Material.Dark
    // Material.accent: Material.Indigo
    // font.capitalization: Font.MixedCase
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 4
            ToolButton {
                text: qsTr("▲")
                onClicked: main.header.visible = false
            }
            ToolButton {
                text: qsTr("1.")
                Layout.preferredWidth: 50
            }
            ToolButton {
                text: qsTr("2.")
                Layout.preferredWidth: 50
            }
            ToolButton {
                text: qsTr("3.")
                Layout.preferredWidth: 50
            }
            Label {
                text: "GRAM 300" // name of the current mnemo
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
            ToolButton {
                text: qsTr("4.")
                Layout.preferredWidth: 50
            }
            ToolButton {
                text: qsTr("5.")
                Layout.preferredWidth: 50
            }
            ToolButton {
                text: qsTr("6.")
                Layout.preferredWidth: 50
            }
            ToolButton {
                text: qsTr("⋮")
                onClicked: drawer.open()
            }
        }
    }
    Drawer {
        id: drawer
        width: main.width
        height: 200
        edge: Qt.BottomEdge
        Rectangle{
            anchors.fill: parent
            color: "gray"
            border.color: "#FFCB08"
            ListView {
                id: settinglist
                anchors.fill: parent
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                    active: ScrollBar.AlwaysOn
                }
                model: ListModel {
                    ListElement {
                        name: "Bill Smith"
                        number: "555 3264"
                    }
                    ListElement {
                        name: "John Brown"
                        number: "555 8426"
                    }
                    ListElement {
                        name: "Sam Wise"
                        number: "555 0473"
                    }
                }
                delegate: TextField {
                    text: name + ": " + number
                    width: settinglist.width
                    height: 40
                    color: "black"
                    font { family: 'Courier'; pixelSize: 20; italic: true; capitalization: Font.SmallCaps }
                    horizontalAlignment: TextInput.AlignLeft
                    selectByMouse: true
                    readOnly: true
                }
                highlight: Rectangle { color: "#e0f2f1"; radius: 5 }
                focus: true
                onCountChanged: {
                    settinglist.currentIndex = settinglist.count-1
                }
            }
        }
    }
    RowLayout { //SplitView
        id: grid
        anchors.fill: parent
        Item{
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 1390
            TabBar {
                id: barMneno
                width: parent.width
                anchors.top: parent.top
                anchors.left: parent.left
                Repeater{
                    id: barMnenoRepeater
                    model: ["Мнемосхема", "Упрощение", "Модель"]
                    TabButton{
                        text: modelData
                        width: Math.max(120, barMneno.width/3)
                        font.pointSize: 12
                    }
                }
            }
            StackLayout {
                id: layoutMneno
                anchors.top: barMneno.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                currentIndex: barMneno.currentIndex
                MnemoBase{
                    id: rectangle
                    // Layout.fillWidth: true
                    // Layout.fillHeight: true
                }
                Lumber{
                    id: lumber
                    // Layout.fillWidth: true
                    // Layout.fillHeight: true
                }
                TestField{
                    id: testMnemo
                    // Layout.fillWidth: true
                    // Layout.fillHeight: true
                }
            }
        }
        Item{
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 500
            TabBar {
                id: barMain
                width: parent.width
                anchors.top: parent.top
                anchors.left: parent.left
                //anchors.right: parent.right
                //height: 100
                Repeater{
                    id: barMainRepeater
                    model: ["Параметры"] // "График А", "Натекание", "Измерение", 
                    TabButton{
                        text: modelData
                        width: Math.max(120, barMain.width) // /4
                        font.pointSize: 12
                    }
                }
            }
            StackLayout {
                id: layoutMain
                anchors.top: barMain.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                currentIndex: barMain.currentIndex
                // ExpCreator{
                //     id: test1
                //     Layout.fillWidth: true
                //     Layout.fillHeight: true
                // }  
                // ChartCreator{
                //     id: test2
                //     Layout.fillWidth: true
                //     Layout.fillHeight: true
                // }
                // ChamberChoose{
                //     id: chamberChooser
                //     Layout.fillWidth: true
                //     // Layout.fillHeight: true
                // }
                // SettingsWindow{
                //     id: test4
                //     Layout.fillWidth: true
                //     Layout.fillHeight: true
                // }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    SettingsConnections{
                    }
                }
            } 
        }
    }
    RoundButton{
        id: openToolBar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 5
        text: qsTr("▼")
        visible: !main.header.visible
        onClicked: main.header.visible = true
    }
    footer: Label {
        text: "swipe me up"
        horizontalAlignment: Text.AlignHCenter
    }
}
