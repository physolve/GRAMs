

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Studio.Components 1.0
import QtQuick.Shapes
import QtQuick.Layouts
import QtQuick.Studio.Effects 1.0

import QtQml.Models

Item {
    id: item1
    width: rectangle.width
    height: rectangle.height
    property var pressureVal: [0.0,0.0,0.0,0.0,0.0,0.0]
                            // 0,  1,  2,  3,  4,  5
    property var tempVal: [0.0,0.0,0.0,0.0,0.0,0.0]
                            // 0,  1,  2,  3,  4
    function setPressureVal(val){
        if(val.length < 6){
            console.log("Not enough values")
            pressureVal = [0.0,0.0,0.0,0.0,0.0,0.0]
        }
        else 
            pressureVal = val
    }
    function setTempVal(val){
        if(val.length < 5){
            console.log("Not enough values")
            tempVal = [0.0,0.0,0.0,0.0,0.0]
        }
        else 
            tempVal = val
    }
    Rectangle {
        id: rectangle
        x: 0
        y: 0
        width: 1320
        height: 900
        color: "#00685757"

        Image {
            id: image
            visible: true
            anchors.fill: parent
            source: "qrc:/GRAMs/pictures/GRAMsMimic.svg"
            sourceSize.height: 2044
            sourceSize.width: 3000
            fillMode: Image.PreserveAspectFit
        }

        GroupBox {
            id: groupBox
            anchors.fill: parent
            bottomPadding: 0
            topPadding: 0
            padding: 0
            anchors.rightMargin: 0
            anchors.bottomMargin: 0
            anchors.leftMargin: 0
            anchors.topMargin: 0
            title: qsTr("GramsMnemo")

            Valve {
                id: valve
                x: 701
                y: 386
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve1
                x: 701
                y: 338
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve2
                x: 701
                y: 281
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve3
                x: 857
                y: 225
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve4
                x: 981
                y: 281
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve5
                x: 925
                y: 157
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve6
                x: 560
                y: 225
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve7
                x: 389
                y: 281
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve8
                x: 389
                y: 338
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve9
                x: 389
                y: 386
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve10
                x: 389
                y: 433
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve11
                x: 524
                y: 486
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve12
                x: 524
                y: 533
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve13
                x: 465
                y: 506
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve14
                x: 323
                y: 579
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve15
                x: 351
                y: 628
                width: 34
                height: 34
                checkable: true
            }

            Valve {
                id: valve16
                x: 377
                y: 673
                width: 34
                height: 34
                checkable: true
            }
        }

        Rectangle {
            id: rectangle1
            x: 252
            y: 342
            width: 90
            height: 30
            color: "#9c3d3d"
            radius: 15
            border.color: "#343232"
        }

        Rectangle {
            id: rectangle2
            x: 252
            y: 388
            width: 90
            height: 30
            color: "#9c3d3d"
            radius: 15
            border.color: "#343232"
        }

        Rectangle {
            id: rectangle3
            x: 252
            y: 435
            width: 90
            height: 30
            color: "#9c3d3d"
            radius: 15
            border.color: "#343232"
        }

        Rectangle {
            id: rectangle4
            x: 570
            y: 488
            width: 70
            height: 30
            color: "#28a3cb"
            radius: 15
            border.color: "#343232"
        }

        Rectangle {
            id: rectangle5
            x: 570
            y: 534
            width: 70
            height: 30
            color: "#28a3cb"
            radius: 15
            border.color: "#343232"
        }

        Rectangle {
            id: rectangle6
            x: 389
            y: 581
            width: 70
            height: 30
            color: "#28a3cb"
            radius: 15
            border.color: "#343232"
        }

        Rectangle {
            id: rectangle7
            x: 435
            y: 674
            width: 70
            height: 30
            color: "#28a3cb"
            radius: 15
            border.color: "#343232"
        }

        Rectangle {
            id: rectangle8
            x: 252
            y: 756
            width: 125
            height: 30
            color: "#28a3cb"
            radius: 15
            border.color: "#343232"
        }

        CircularSlider {
            id: aDD311_s
            x: 476
            y: 173
            width: 80
            height: 80
            //onValueChanged: item1.prVal[0] = value
            handle: Rectangle {
                id: handleItem1
                width: 6
                height: 6
                color: "#908990"
                radius: width / 2
                border.color: "#fefefe"
                border.width: 5
                antialiasing: true
                transform: [
                    Translate {
                        x: (aDD311_s.handleWidth - width) / 2
                        y: (aDD311_s.handleHeight - height) / 2
                    }
                ]
            }
            minValue: 0
            rotation: 180
            progressWidth: 10
            startAngle: 40
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.pressureVal[0]
            maxValue: 120
            endAngle: 320

            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: aDD311_s.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                rotation: 180
                font.pointSize: 13
            }
        }
        
        CircularSlider {
            id: aDD312_s
            x: 539
            y: 122
            width: 80
            height: 80
            //onValueChanged: item1.prVal[1] = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: aDD312_s.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.pressureVal[1]
            maxValue: 120
            progressWidth: 10
            handle: Rectangle {
                id: handleItem2
                width: 6
                height: 6
                color: "#908990"
                radius: width / 2
                border.color: "#fefefe"
                border.width: 5
                transform: [
                    Translate {
                        x: (aDD312_s.handleWidth - width) / 2
                        y: (aDD312_s.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: aDD331_s
            x: 763
            y: 165
            width: 80
            height: 80
            //onValueChanged: item1.prVal[2] = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: aDD331_s.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            trackWidth: 10
            Layout.fillWidth: false
            value: item1.pressureVal[2]
            progressWidth: 10
            maxValue: 120
            handle: Rectangle {
                id: handleItem3
                width: 6
                height: 6
                color: "#908990"
                radius: width / 2
                border.color: "#fefefe"
                border.width: 5
                transform: [
                    Translate {
                        x: (aDD331_s.handleWidth - width) / 2
                        y: (aDD331_s.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: aDD332_s
            x: 835
            y: 68
            width: 80
            height: 80
            //onValueChanged: item1.prVal[3] = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: aDD332_s.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.pressureVal[3]
            maxValue: 120
            progressWidth: 10
            handle: Rectangle {
                id: handleItem4
                width: 6
                height: 6
                color: "#908990"
                radius: width / 2
                border.color: "#fefefe"
                border.width: 5
                transform: [
                    Translate {
                        x: (aDD332_s.handleWidth - width) / 2
                        y: (aDD332_s.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: aDD333_s
            x: 965
            y: 48
            width: 80
            height: 80
            //onValueChanged: item1.prVal[4] = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: aDD333_s.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            trackWidth: 10
            Layout.fillWidth: false
            value: item1.pressureVal[4]
            progressWidth: 10
            maxValue: 120
            handle: Rectangle {
                id: handleItem5
                width: 6
                height: 6
                color: "#908990"
                radius: width / 2
                border.color: "#fefefe"
                border.width: 5
                transform: [
                    Translate {
                        x: (aDD333_s.handleWidth - width) / 2
                        y: (aDD333_s.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: aDD391_s
            x: 524
            y: 575
            width: 80
            height: 80
            //onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: aDD391_s.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.pressureVal[5]
            maxValue: 120
            progressWidth: 10
            handle: Rectangle {
                id: handleItem6
                width: 6
                height: 6
                color: "#908990"
                radius: width / 2
                border.color: "#fefefe"
                border.width: 5
                transform: [
                    Translate {
                        x: (aDD391_s.handleWidth - width) / 2
                        y: (aDD391_s.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: aDV301_s
            x: 220
            y: 474
            width: 80
            height: 80
            //onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: aDV301_s.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            trackWidth: 10
            Layout.fillWidth: false
            value: 0
            progressWidth: 10
            maxValue: 120
            handle: Rectangle {
                id: handleItem7
                width: 6
                height: 6
                color: "#908990"
                radius: width / 2
                border.color: "#fefefe"
                border.width: 5
                transform: [
                    Translate {
                        x: (aDV301_s.handleWidth - width) / 2
                        y: (aDV301_s.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }
        CircularSlider {
            id: aDT314_s
            x: 577
            y: 363
            width: 80
            height: 80
            //onValueChanged: item1.newTempVal = value
            handleWidth: 10
            hideTrack: true
            Item {
                anchors.fill: parent
                anchors.margins: 5
                Rectangle {
                    id: mask4
                    color: "#282a36"
                    radius: width / 2
                    border.color: "#44475a"
                    border.width: 5
                    anchors.fill: parent
                }

                Item {
                    anchors.fill: mask4
                    anchors.margins: 5
                    layer.effect: OpacityMaskEffect {
                        id: opacityMask4
                        maskSource: mask4
                    }
                    layer.enabled: true
                    rotation: 180
                    Rectangle {
                        width: parent.width
                        height: parent.height * aDT314_s.value/1200
                        color: "#ec0636"
                        layer.enabled: true
                    }
                }

                Label {
                    color: "#fefefe"
                    text: (aDT314_s.value).toFixed() + " °C"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            value: item1.tempVal[0]
            hideProgress: true
            handleColor: "#6272a4"
            handleHeight: 10
        }
        CircularSlider {
            id: aDT352_s
            x: 651
            y: 463
            width: 80
            height: 80
            //onValueChanged: item1.newTempVal = value
            handleWidth: 10
            hideTrack: true
            Item {
                anchors.fill: parent
                anchors.margins: 5
                Rectangle {
                    id: mask3
                    color: "#282a36"
                    radius: width / 2
                    border.color: "#44475a"
                    border.width: 5
                    anchors.fill: parent
                }

                Item {
                    anchors.fill: mask3
                    anchors.margins: 5
                    layer.effect: OpacityMaskEffect {
                        id: opacityMask3
                        maskSource: mask3
                    }
                    layer.enabled: true
                    rotation: 180
                    Rectangle {
                        width: parent.width
                        height: parent.height * aDT352_s.value/1200
                        color: "#ec0636"
                        layer.enabled: true
                    }
                }

                Label {
                    color: "#fefefe"
                    text: (aDT352_s.value).toFixed() + " °C"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            value: item1.tempVal[1]
            hideProgress: true
            handleColor: "#6272a4"
            handleHeight: 10
        }
        CircularSlider {
            id: aDT354_s
            x: 710
            y: 509
            width: 80
            height: 80
            //onValueChanged: item1.newTempVal = value
            handleHeight: 10
            hideTrack: true
            value: item1.tempVal[2]
            Item {
                anchors.fill: parent
                anchors.margins: 5
                Rectangle {
                    id: mask2
                    color: "#282a36"
                    radius: width / 2
                    border.color: "#44475a"
                    border.width: 5
                    anchors.fill: parent
                }

                Item {
                    anchors.fill: mask2
                    anchors.margins: 5
                    Rectangle {
                        width: parent.width
                        height: parent.height * aDT354_s.value/1200
                        color: "#ec0636"
                        layer.enabled: true
                    }
                    layer.effect: OpacityMaskEffect {
                        id: opacityMask2
                        maskSource: mask2
                    }
                    rotation: 180
                    layer.enabled: true
                }

                Label {
                    color: "#fefefe"
                    text: (aDT354_s.value).toFixed()  + " °C"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            handleWidth: 10
            hideProgress: true
            handleColor: "#6272a4"
        }
        CircularSlider {
            id: aDT358_s
            x: 835
            y: 332
            width: 80
            height: 80
            //onValueChanged: item1.newTempVal = value
            handleHeight: 10
            hideTrack: true
            value: item1.tempVal[3]
            Item {
                anchors.fill: parent
                anchors.margins: 5
                Rectangle {
                    id: mask1
                    color: "#282a36"
                    radius: width / 2
                    border.color: "#44475a"
                    border.width: 5
                    anchors.fill: parent
                }

                Item {
                    anchors.fill: mask1
                    anchors.margins: 5
                    Rectangle {
                        width: parent.width
                        height: parent.height * aDT358_s.value/1200
                        color: "#ec0636"
                        layer.enabled: true
                    }
                    layer.effect: OpacityMaskEffect {
                        id: opacityMask1
                        maskSource: mask1
                    }
                    rotation: 180
                    layer.enabled: true
                }

                Label {
                    color: "#fefefe"
                    text: (aDT358_s.value).toFixed() + " °C"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            handleWidth: 10
            hideProgress: true
            handleColor: "#6272a4"
        }
        CircularSlider {
            id: aDT359_s
            x: 1044
            y: 263
            hideProgress: true
            hideTrack: true
            width: 80
            height: 80
            //onValueChanged: item1.newTempVal = value
            handleColor: "#6272A4"
            handleWidth: 10
            handleHeight: 10
            value: item1.tempVal[4]

            // Custom progress Indicator
            Item {
                anchors.fill: parent
                anchors.margins: 5
                Rectangle {
                    id: mask
                    anchors.fill: parent
                    radius: width / 2
                    color: "#282A36"
                    border.width: 5
                    border.color: "#44475A"
                }

                Item {
                    anchors.fill: mask
                    anchors.margins: 5
                    layer.enabled: true
                    rotation: 180
                    layer.effect: OpacityMaskEffect {
                        id: opacityMask
                        maskSource: mask
                    }
                    Rectangle {
                        height: parent.height * aDT359_s.value/1200
                        width: parent.width
                        //radius: parent.width/2
                        layer.enabled: true
                        color: "#ec0636"
                    }
                }

                Label {
                    anchors.centerIn: parent
                    font.pointSize: 8
                    color: "#FEFEFE"
                    text: (aDT359_s.value).toFixed() + " °C"
                }
            }
        }
    }
    // Binding{
    //     target: item1
    //     property: "newVal"
    //     value: _myModel.data(0,260)
    // }
}
/*##^##
Designer {
    D{i:0;formeditorZoom:0.5}
}
##^##*/

