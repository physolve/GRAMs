import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.chamberChooserSingleton 1.0

Window {
    id: root
    color: "#2B2B2B"
    GridLayout{
        anchors.fill: parent
        flow: GridLayout.LeftToRight
        Label {
            text: "Имя камеры"
            font.pixelSize: 22
            font.italic: true
        }
        TextField{
            readOnly: true
            text: ChamberChooser.chamberName
        }
        Label {
            text: "Объем, см3"
            font.pixelSize: 22
            font.italic: true
        }
        TextField{
            readOnly: true
            text: ChamberChooser.volume
        }
    }
}