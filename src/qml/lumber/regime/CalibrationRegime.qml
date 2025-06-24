import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Grams.backendSourceSingleton 1.0
import Grams.addRemoveQuartileSingleton 1.0

Window {
    id: root
    color: "#2B2B2B"
    flags: Qt.Dialog
    Row{
        x: 5
        y: 25
        width: parent.width - 10
        spacing: 1
        Button{
            text: "Калибровка"
            onClicked: {
                if(gasPortChoose.currentIndex == 0){
                    gasPortNotChosen.open()
                    checked = false
                    return
                }
                Grams.testActionHandler()
            }
        }
        ComboBox{
            id: gasPortChoose
            model: ["ED2"]
            implicitContentWidthPolicy: ComboBox.ContentItemImplicitWidth
            onActivated: {
                
            }
        }
    }
    Row{
        x: 5
        y: 125
        spacing: 10
        Text{
            text: "E  "
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
            text: "0"//PlayPressure.guiPresTotal.prSQ.toFixed(3)
            // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            // onAcceptableInputChanged:
            //     color = acceptableInput ? "white" : "#D94625";
        }
        Text{
            text: "D2"
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
            text: "0"//PlayPressure.guiPresTotal.prSC1.toFixed(3)
            // validator: DoubleValidator { bottom: 0; top: eSupply.topPressure}
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            // onAcceptableInputChanged:
            //     color = acceptableInput ? "white" : "#D94625";
        }
        Text{
            text: "ED2"
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
            text: "0"//PlayPressure.guiPresTotal.prSC1.toFixed(3)
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