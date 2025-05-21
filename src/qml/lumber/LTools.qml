import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.backendSourceSingleton 1.0

Rectangle {
    id: lTools
    color: "#464646"
    border.color: "#F2C029"
    anchors.fill: parent
    // reminder to change for other systems
    property double topPressure: 50 
    signal openExpSettings()
    Row{
        x: 5
        y: 5
        spacing: 10
        Button{
            id: expSettingsBtn
            width: 200
            height: 50
            text: "Параметры эксперимента"
            // onClicked: lChamber.pickChamber()
        }
        Label {
            color: "White"
            font.family: "Verdana"
            text: "Автоматическая подача газа в камеру" // changing to exp function
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 12
        }
    }
    Button{
        x: 25
        y: 250
        text: "Проверка play"
        onClicked: Grams.testPlayPressure()
    }
    Column{
        x: 25
        y: 130
        spacing: 25
        Text{
            // width: parent.width
            text: "Давление в камере"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            // anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            validator: DoubleValidator { bottom: 1e-6; top: lTools.topPressure}
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }
    Column{
        x: 225
        y: 130
        spacing: 25
        Text{
            // width: parent.width
            text: "Давление в эталонном резервуаре"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            // anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            readOnly: true
            // text: "?"
            validator: DoubleValidator { bottom: 1e-6; top: lTools.topPressure}
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }
    Column{
        x: 500
        y: 130
        spacing: 25
        Text{
            // width: parent.width
            text: "Количество напусков"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            // anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            readOnly: true
            placeholderText: "раз"
            text: "?"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }
}