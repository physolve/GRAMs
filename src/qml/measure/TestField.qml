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
        width: parent.width
        height: parent.height/2 - 10
        flow: GridView.FlowLeftToRight
        cellWidth: 220; cellHeight: 160
        model: TestFieldBack.guiNodes
        clip: true
        interactive: false
        delegate: Rectangle { 
            width: 200
            height: 150
            color:"transparent"; border.color: "#464646";
            GridLayout{
                anchors.fill: parent
                anchors.margins: 5
                columns: 2
                rowSpacing: 5
                uniformCellWidths: true
                Row{
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 5
                    Layout.fillWidth: true
                    Text { text: `${modelData.aName}:`; font.pointSize: 10; color: "white"; font.family: "Verdana" }
                    Text{
                        text: modelData.prA + " бар"
                        font.pointSize: 10; color: "white"; font.family: "Verdana"
                    }
                }
                Row{
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 5
                    Text { text: `${modelData.bName}:`; font.pointSize: 10; color: "white"; font.family: "Verdana" }
                    Label{
                        text: modelData.prB + " бар"
                        font.pointSize: 10; color: "white"; font.family: "Verdana" 
                    }
                }
                Row{
                    Layout.fillWidth: true
                    spacing: 5
                    Text { text: `${modelData.nodeName}: `; font.pointSize: 10; color: "white"; font.family: "Verdana" }
                    Label{
                        // readOnly: true
                        text: modelData.prC + " бар"
                        font.pointSize: 10; color: "white"; font.family: "Verdana" 
                    }
                }
                Button{
                    id: collapseNode // between two
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 40
                    Layout.margins: 10
                    Layout.alignment: Qt.AlignHCenter
                    background: Rectangle {
                        color: "white"
                        border.width: 1
                        border.color: "blue"
                        radius: parent.width/4
                    }
                    icon.source: "qrc:/collapseSVG.svg"
                    icon.color: pressed ? "red" : "black"
                    onClicked: {
                        TestFieldBack.runCollapse(index)
                    }
                }
            }
        }
    }
    GridView {
        id: viewCollapsed
        y: parent.height/2 + 5
        width: parent.width
        height: parent.height/2 - 10
        flow: GridView.FlowLeftToRight
        cellWidth: 220; cellHeight: 160
        model: TestFieldBack.guiCollapsed
        clip: true
        interactive: false
        delegate: Rectangle { 
            width: 200
            height: 70
            color:"transparent"; border.color: "#464646";
            GridLayout{
                width: parent.width
                height: parent.height
                columns: 2
                rowSpacing: 5
                uniformCellWidths: true
                Text { text: `${modelData}`; font.pointSize: 13; color: "white";
                font.family: "Verdana"; Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter }
                Button{
                    id: splitNode // between two
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 40
                    Layout.margins: 10
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    background: Rectangle {
                        color: "white"
                        border.width: 1
                        border.color: "blue"
                        radius: parent.width/4
                    }
                    icon.source: "qrc:/splitSVG.svg"
                    icon.color: pressed ? "red" : "black"
                    onClicked: {
                        TestFieldBack.runSplit(modelData)
                    }
                }
            }
        }
    }
}