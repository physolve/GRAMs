import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.chamberChooserSingleton 1.0

Item {
    id: root
    // color: "#2B2B2B"
    GridLayout{
        width: parent.width
        height: 70*3
        flow: GridLayout.LeftToRight
        rowSpacing: 20
        columnSpacing: 10
        columns: width/250
        uniformCellWidths: true
        Row{
            spacing: 15
            Layout.fillWidth: true
            Label {
                text: "Имя камеры"
                font.pointSize: 12; color: "white" 
                font.family: "Verdana"
                anchors.verticalCenter: parent.verticalCenter
            }
            TextField{
                readOnly: true
                text: ChamberChooser.chamberParams.chamberName
                font { family: 'Courier'; pixelSize: 16; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
            }
        }
        Row{
            spacing: 15
            Layout.fillWidth: true
            Label {
                text: "Объем, см3"
                font.pointSize: 12; color: "white" 
                font.family: "Verdana"
                anchors.verticalCenter: parent.verticalCenter
            }
            TextField{
                readOnly: true
                text: ChamberChooser.chamberParams.volume
                font { family: 'Courier'; pixelSize: 16; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
            }   
        }
        Row{
            spacing: 15
            Layout.fillWidth: true
            Label {
                text: "Состояние"
                font.pointSize: 12; color: "white" 
                font.family: "Verdana"
                anchors.verticalCenter: parent.verticalCenter
            }
            TextField{
                readOnly: true
                text: ChamberChooser.chamberParams.status ? "Открыта" : " Закрыта"
                height: 40
                font { family: 'Courier'; pixelSize: 16; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
            }   
        }
    }
}