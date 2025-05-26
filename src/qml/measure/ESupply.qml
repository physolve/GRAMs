import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.playPressureSingleton 1.0

Rectangle {
    id: eSupply
    color: "#464646"
    border.color: "#F2C029"
    anchors.fill: parent
    // reminder to change for other systems
    property double topPressure: 50 
    Button{
        x: 25
        y: 5
        text: "Проверка play"
        onClicked: PlayPressure.testPlayPressure() //?
    }
    Column{
        x: 25
        y: 130
        spacing: 15
        Text{
            // width: parent.width
            text: "Цель. давл. в камере"
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
            validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
            text: PlayPressure.guiPresTarget.chPresTg.toFixed(3)
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            onEditingFinished: PlayPressure.guiPresTarget.chPresTg = text
            onAcceptableInputChanged:
                color = acceptableInput ? "white" : "#D94625";
        }
    }
    Column{
        x: 25
        y: 225
        spacing: 15
        Text{
            // width: parent.width
            text: "Нач. давл. в камере"
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
            validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
            text: PlayPressure.guiPresTarget.chPresInit.toFixed(3)
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            onEditingFinished: PlayPressure.guiPresTarget.chPresInit = text
            onAcceptableInputChanged:
                    color = acceptableInput ? "white" : "#D94625";
        }
    }
    // начальное давление в камере
    Column{
        x: 225
        y: 130
        spacing: 15
        Text{
            // width: parent.width
            text: "Давл. в эталонном резервуаре"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            // anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        Row{
            spacing: 10
            Text{
                text: "B  "
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
                text: PlayPressure.guiPresTarget.stPresTg.toFixed(3)
                validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                onAcceptableInputChanged:
                    color = acceptableInput ? "white" : "#D94625";
            }
            Text{
                text: "C1"
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
                text: PlayPressure.guiPresTarget.c1PresTg.toFixed(3)
                validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                onAcceptableInputChanged:
                    color = acceptableInput ? "white" : "#D94625";
            }
        }
        Row{
            spacing: 10
            Text{
                text: "C2"
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
                text: PlayPressure.guiPresTarget.c2PresTg.toFixed(3)
                validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                onAcceptableInputChanged:
                    color = acceptableInput ? "white" : "#D94625";
            }
            Text{
                text: "C3"
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
                text: PlayPressure.guiPresTarget.c3PresTg.toFixed(3)
                validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                onAcceptableInputChanged:
                    color = acceptableInput ? "white" : "#D94625";
            }
        }
    }
    Column{
        x: 25
        y: 310
        spacing: 15
        Text{
            // width: parent.width
            text: "Количество подач"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            // readOnly: true
            placeholderText: "раз"
            text: PlayPressure.guiPresTarget.nAccum
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            onEditingFinished: PlayPressure.guiPresTarget.nAccum = text
        }
    }
    Column{
        x: 225
        y: 310
        spacing: 15
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
            text: PlayPressure.guiPresTarget.supplyCount
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }

}