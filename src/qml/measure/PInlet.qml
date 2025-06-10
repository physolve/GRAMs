import QtQuick
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0
// import Grams.actionHandlerSingleton 1.0
import Grams.addRemoveQuartileSingleton 1.0
import Grams.initSourceSingleton 1.0

Item {
    id: root
    // color: "#2B2B2B"
    // reminder to change for other systems
    property double topPressure: 50 // from profile
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
            text: AddRemoveQuar.inletStrategy.reducerLimit
            validator: IntValidator { bottom: 0; top: 500}
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            onEditingFinished: AddRemoveQuar.inletStrategy.reducerLimit = text
            onAcceptableInputChanged:
                color = acceptableInput ? "white" : "#D94625";
        }
    }
    Row{
        x: 25
        y: 100
        spacing: 10
        Text{
            // width: parent.width
            text: "Порт"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        ComboBox{
            id: gasPortChoose
            model: InitSource.addRemoveQuar.gasSupplyValves
            Component.onCompleted: currentIndex = indexOfValue(AddRemoveQuar.inletStrategy.usePort)
            onActivated: {
                AddRemoveQuar.inletStrategy.usePort = gasPortChoose.currentText
            }
        }
    }
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
            width: 75
            height: 35
            text: AddRemoveQuar.inletStrategy.pressureLimit.toFixed(2)
            validator: DoubleValidator { bottom: 0; top: root.topPressure; decimals: 2}
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            onEditingFinished: AddRemoveQuar.inletStrategy.pressureLimit = text
            onAcceptableInputChanged:
                color = acceptableInput ? "white" : "#D94625";
        }
    }
    Row{
        x: 25
        y: 230
        spacing: 10
        Text{
            // width: parent.width
            text: "Открыть на время"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            width: 110
            text: AddRemoveQuar.inletStrategy.openTime
            validator: IntValidator { bottom: 1000; top: 30000}
            placeholderText: "мс"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            onEditingFinished: AddRemoveQuar.inletStrategy.openTime = text
            onAcceptableInputChanged:
                color = acceptableInput ? "white" : "#D94625";
        }
    }
}