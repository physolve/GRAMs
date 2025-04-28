import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.testFieldSingleton 1.0

Item {
    id: root
    // color: "#2B2B2B"
    height: 600
    width: 900
    GridView {
        id: viewTestField
        anchors.fill: parent
        // height: cellHeight * 3 
        flow: GridView.FlowLeftToRight
        // anchors.topMargin:10
        // anchors.leftMargin: 5
        cellWidth: 140; cellHeight: 110
        model: TestFieldBack.guiNodes
        clip: true
        interactive: false
        delegate: Rectangle { 
            width: 140
            height: 100
            color:"transparent"; border.color: "#464646";
            GridLayout{
                anchors.fill: parent
                anchors.margins: 5
                // flow: GridLayout.LeftToRight
                columns: 2
                rowSpacing: 5
                uniformCellWidths: true
                Row{
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 5
                    Layout.fillWidth: true
                    Text { text: `${modelData.aName}:`; font.pointSize: 9; color: "white"; 
                        font.family: "Verdana"
                    }
                    Label{
                        // readOnly: true
                        text: modelData.prA + " бар"
                        font { family: 'Courier'; pointSize: 9; }
                        horizontalAlignment: TextInput.AlignHCenter
                        // selectByMouse: true
                    }
                }
                Row{
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 5
                    Text { text: `${modelData.bName}:`; font.pointSize: 9; color: "white"; 
                        font.family: "Verdana"
                    }
                    Label{
                        // readOnly: true
                        text: modelData.prB + " бар"
                        font { family: 'Courier'; pointSize: 9; }
                        horizontalAlignment: TextInput.AlignHCenter
                        // selectByMouse: true
                    }
                }
                Row{
                    Layout.fillWidth: true
                    spacing: 5
                    Text { text: `${modelData.nodeName}: `; font.pointSize: 9; color: "white"; 
                        font.family: "Verdana"
                    }
                    Label{
                        // readOnly: true
                        text: modelData.prC + " бар"
                        font { family: 'Courier'; pointSize: 9; }
                        horizontalAlignment: TextInput.AlignHCenter
                        // selectByMouse: true
                    }
                }
                Button{
                    id: collapseNode
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.margins: 10
                    Layout.alignment: Qt.AlignHCenter
                    background: Rectangle {
                        color: "white"
                        border.width: 1
                        border.color: "blue"
                        radius: parent.width/4
                    }
                    icon.source: "qrc:/SomeAliasedSVG.svg"
                    icon.color: pressed ? "red" : "blacks"

                    // icon.width: 25
                    // icon.height: 25
                }
                // Image{
                //     id:img
                //     // anchors.fill: parent
                //     Layout.preferredHeight: 20
                //     Layout.preferredWidth: 20
                //     source: "qrc:/SomeAliasedSVG.svg"
                // }
            }
        }
    }
}