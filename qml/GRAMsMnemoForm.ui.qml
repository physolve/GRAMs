

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
    property real newVal: 10
    property real newTempVal: 0.5
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
            source: "qrc:/MyApplication/pictures/GRAMsMimic.svg"
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
            id: slider1
            x: 476
            y: 173
            width: 80
            height: 80
            //onValueChanged: item1.newVal = value
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
                        x: (slider1.handleWidth - width) / 2
                        y: (slider1.handleHeight - height) / 2
                    }
                ]
            }
            minValue: 0
            rotation: 180
            progressWidth: 10
            startAngle: 40
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.newVal
            
            maxValue: 120
            endAngle: 320

            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: slider1.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                rotation: 180
                font.pointSize: 13
            }
        }
        
        CircularSlider {
            id: slider2
            x: 539
            y: 122
            width: 80
            height: 80
            onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: slider2.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.newVal
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
                        x: (slider2.handleWidth - width) / 2
                        y: (slider2.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: slider3
            x: 763
            y: 165
            width: 80
            height: 80
            onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: slider3.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            trackWidth: 10
            Layout.fillWidth: false
            value: item1.newVal
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
                        x: (slider3.handleWidth - width) / 2
                        y: (slider3.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: slider4
            x: 835
            y: 68
            width: 80
            height: 80
            onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: slider4.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.newVal
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
                        x: (slider4.handleWidth - width) / 2
                        y: (slider4.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: slider5
            x: 965
            y: 48
            width: 80
            height: 80
            onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: slider5.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            trackWidth: 10
            Layout.fillWidth: false
            value: item1.newVal
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
                        x: (slider5.handleWidth - width) / 2
                        y: (slider5.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: slider6
            x: 524
            y: 575
            width: 80
            height: 80
            onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: slider6.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Layout.fillWidth: false
            trackWidth: 10
            value: item1.newVal
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
                        x: (slider6.handleWidth - width) / 2
                        y: (slider6.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: slider7
            x: 220
            y: 474
            width: 80
            height: 80
            onValueChanged: item1.newVal = value
            minValue: 0
            endAngle: 320
            rotation: 180
            Label {
                width: 40
                height: 20
                color: "#fefefe"
                text: slider7.value.toFixed()
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 13
                rotation: 180
                anchors.horizontalCenter: parent.horizontalCenter
            }
            trackWidth: 10
            Layout.fillWidth: false
            value: item1.newVal
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
                        x: (slider7.handleWidth - width) / 2
                        y: (slider7.handleHeight - height) / 2
                    }
                ]
                antialiasing: true
            }
            startAngle: 40
        }

        CircularSlider {
            id: customSlider
            x: 1044
            y: 263
            hideProgress: true
            hideTrack: true
            width: 80
            height: 80
            onValueChanged: item1.newTempVal = value
            handleColor: "#6272A4"
            handleWidth: 10
            handleHeight: 10
            value: item1.newTempVal

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
                        height: parent.height * customSlider.value
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
                    text: (customSlider.value * 100).toFixed() + "%"
                }
            }
        }

        CircularSlider {
            id: customSlider1
            x: 835
            y: 332
            width: 80
            height: 80
            onValueChanged: item1.newTempVal = value
            handleHeight: 10
            hideTrack: true
            value: item1.newTempVal
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
                        height: parent.height * customSlider1.value
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
                    text: (customSlider1.value * 100).toFixed() + "%"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            handleWidth: 10
            hideProgress: true
            handleColor: "#6272a4"
        }

        CircularSlider {
            id: customSlider2
            x: 710
            y: 509
            width: 80
            height: 80
            onValueChanged: item1.newTempVal = value
            handleHeight: 10
            hideTrack: true
            value: item1.newTempVal
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
                        height: parent.height * customSlider2.value
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
                    text: (customSlider2.value * 100).toFixed() + "%"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            handleWidth: 10
            hideProgress: true
            handleColor: "#6272a4"
        }

        CircularSlider {
            id: customSlider3
            x: 651
            y: 463
            width: 80
            height: 80
            onValueChanged: item1.newTempVal = value
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
                        height: parent.height * customSlider3.value
                        color: "#ec0636"
                        layer.enabled: true
                    }
                }

                Label {
                    color: "#fefefe"
                    text: (customSlider3.value * 100).toFixed() + "%"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            value: item1.newTempVal
            hideProgress: true
            handleColor: "#6272a4"
            handleHeight: 10
        }

        CircularSlider {
            id: customSlider4
            x: 577
            y: 363
            width: 80
            height: 80
            onValueChanged: item1.newTempVal = value
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
                        height: parent.height * customSlider4.value
                        color: "#ec0636"
                        layer.enabled: true
                    }
                }

                Label {
                    color: "#fefefe"
                    text: (customSlider4.value * 100).toFixed() + "%"
                    anchors.centerIn: parent
                    font.pointSize: 8
                }
            }
            value: item1.newTempVal
            hideProgress: true
            handleColor: "#6272a4"
            handleHeight: 10
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

