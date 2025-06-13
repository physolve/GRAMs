import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.playPressureSingleton 1.0
import Grams.backendSourceSingleton 1.0

Rectangle {
    id: eSupply
    color: "#464646"
    border.color: "#F2C029"
    // anchors.fill: parent
    // reminder to change for other systems
    property double topPressure: 50 // from profile
    Button{
        x: 25
        y: 5
        text: "Проверка расчета целевого"
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
            text: PlayPressure.guiPresTarget.chPresTg.toFixed(3)
            validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
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
            text: PlayPressure.guiPresTarget.chPresInit.toFixed(3)
            validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
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
    Column{
        x: 25
        y: 400
        spacing: 15
        Text{
            // width: parent.width
            text: "Текущее давление в камере"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
            color: "white"
        }
        Row{
            spacing: 10
            Text{
                text: "E "
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
                text: Grams.guiPresVirtual.prRF.toFixed(3)
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
        x: 225
        y: 400
        spacing: 15
        Text{
            // width: parent.width
            text: "Текущее давление в эталонном"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
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
                text: Grams.guiPresVirtual.prSQ.toFixed(3)
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
                text: Grams.guiPresVirtual.prSC1.toFixed(3)
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
                text: Grams.guiPresVirtual.prSC2.toFixed(3)
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
                text: Grams.guiPresVirtual.prSC3.toFixed(3)
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
        y: 540
        spacing: 15
        Text{
            // width: parent.width
            text: "Промежуточное в \n реакционном, моль"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
            color: "white"
        }
        Row{
            spacing: 10
            Text{
                text: "E "
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
                text: PlayPressure.guiPresChange.prRQ.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
            }
        }
    }
    Column{
        x: 225
        y: 540
        spacing: 15
        Text{
            // width: parent.width
            text: "Промежуточное в \n эталонном, моль"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
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
                text: PlayPressure.guiPresChange.prSQ.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
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
                text: PlayPressure.guiPresChange.prSC1.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
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
                text: PlayPressure.guiPresChange.prSC2.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
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
                text: PlayPressure.guiPresChange.prSC3.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
            }
        }
    }
    Column{
        x: 25
        y: 690
        spacing: 15
        Text{
            // width: parent.width
            text: "Добавить в \n реакционный, моль"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
            color: "white"
        }
        Row{
            spacing: 10
            Text{
                text: "E "
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
                text: PlayPressure.guiPresTotal.prRQ.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
            }
        }
    }
    Column{
        x: 225
        y: 690
        spacing: 15
        Text{
            // width: parent.width
            text: "Добавить в \n эталонный, моль"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
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
                text: PlayPressure.guiPresTotal.prSQ.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
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
                text: PlayPressure.guiPresTotal.prSC1.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
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
                text: PlayPressure.guiPresTotal.prSC2.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
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
                text: PlayPressure.guiPresTotal.prSC3.toFixed(3)
                // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
                placeholderText: "бар"
                font { family: 'Courier'; pointSize: 10; }
                horizontalAlignment: TextInput.AlignHCenter
                selectByMouse: true
                // onAcceptableInputChanged:
                //     color = acceptableInput ? "white" : "#D94625";
            }
        }
    }
}