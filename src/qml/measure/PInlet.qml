import QtQuick
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0

Item {
    id: root
    // color: "#2B2B2B"
    Row{
        x: 25
        y: 170
        spacing: 10
        Text{
            // width: parent.width
            text: "Целевое (настройка)"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            // readOnly: true
            text: "0"
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }
    Row{
        x: 25
        y: 25
        spacing: 10
        Text{
            // width: parent.width
            text: "Давление на редукторе"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            readOnly: true
            text: "0"
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }
}