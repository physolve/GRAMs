import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.chamberChooserSingleton 1.0
import Grams.backendSourceSingleton 1.0

Item {
    id: root
    // color: "#2B2B2B"
    GridLayout{
        width: parent.width
        height: 70*3
        flow: GridLayout.LeftToRight
        rowSpacing: 20
        columnSpacing: 10
        columns: 4//width/250
        // uniformCellWidths: true
        Label {
            text: "Имя камеры"
            font.pointSize: 12; color: "white" 
            font.family: "Verdana"
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            readOnly: true
            text: ChamberChooser.chamberParams.chamberName
            font { family: 'Courier'; pixelSize: 16; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
        Label {
            text: "Объем, см3"
            font.pointSize: 12; color: "white" 
            font.family: "Verdana"
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            readOnly: true
            text: ChamberChooser.chamberParams.volume
            font { family: 'Courier'; pixelSize: 16; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }   
        Label {
            text: "Состояние"
            font.pointSize: 12; color: "white" 
            font.family: "Verdana"
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            readOnly: true
            text: ChamberChooser.chamberParams.status ? "Открыта" : " Закрыта"
            // height: 40
            font { family: 'Courier'; pixelSize: 16; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
        Label{
            // width: 80
            text: "Тигель:"
            font { family: 'Courier'; pixelSize: 16; }
            // anchors.verticalCenter: parent.verticalCenter
        }
        ComboBox{
            id: sampleChoose
            Layout.preferredWidth: 110
            // height: 40
            model: ["a","b","c"]
            font { family: 'Courier'; pixelSize: 16; }
            onActivated: {
            }
        }
        Button{
            id: openChamberTool
            checkable: true
            checked: ChamberChooser.chamberParams.status
            text: checked ? "Закрыть камеру" : "Открыть камеру"
            onClicked: Grams.setManualChamberValve(checked)
        }
    }
}