

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Studio.Effects 1.0
import QtQuick.Studio.Components 1.0
import QtQuick.Shapes 1.0

Button {
    id: control
    flat: false
    hoverEnabled: true
    icon.cache: false
    icon.color: "#00ffffff"
    layer.enabled: false

    property bool value: true
    property string name: ""

    // pressAndHold

    implicitWidth: buttonBackground.implicitWidth
    implicitHeight: buttonBackground.implicitHeight
    leftPadding: 4
    rightPadding: 4

    bottomInset: 0
    topInset: 0

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        implicitWidth: 34
        implicitHeight: 34
        opacity: enabled ? 1 : 0.3
        color: "#aacba4"
        radius: 17
        border.color: "#8a938f"
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter

        Rectangle {
            id: rectangle
            x: 5
            y: 5
            width: 24
            height: 24
            radius: 12
            border.color: "#9a9a9a"
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 0
            rotation: -90

            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop {
                    id: gradientStop
                    position: 0
                    color: "#8c8c8c"
                }

                GradientStop {
                    id: gradientStop1
                    position: 1
                    color: "#f8f0f0"
                }
            }
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
    states: [
        State {
            name: "down"
            when: control.down

            PropertyChanges {
                target: buttonBackground
                color: "#d4d582"
            }
        },
        State {
            name: "cheked"
            when: control.checked

            PropertyChanges {
                target: buttonBackground
                color: "#49e92c"
            }

            PropertyChanges {
                target: rectangle
                border.color: "#507750"
            }
        },
        State {
            name: "unchecked"
            when: !control.checked
        }
    ]
    transitions: [
        Transition {
            id: unchToDown
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop1
                        property: "color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "border.color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "border.color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "rotation"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop
                        property: "color"
                        duration: 200
                    }
                }
            }
            to: "down,unchecked"
            from: "unchecked"
        },
        Transition {
            id: toOn
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop1
                        property: "color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "border.color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "border.color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "rotation"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop
                        property: "color"
                        duration: 200
                    }
                }
            }
            to: "cheked"
            from: "down,unchecked"
        },
        Transition {
            id: toOff1

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "border.color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "rotation"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop1
                        property: "color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "border.color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop
                        property: "color"
                        duration: 200
                    }
                }
            }
            to: "cheked"
            from: "hover,unchecked"
        },
        Transition {
            id: chToDown

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "border.color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "rotation"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop1
                        property: "color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "border.color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop
                        property: "color"
                        duration: 200
                    }
                }
            }
            to: "down,cheked"
            from: "cheked"
        },
        Transition {
            id: toOff

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "border.color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: rectangle
                        property: "rotation"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop1
                        property: "color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "color"
                        duration: 200
                    }
                }

                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: buttonBackground
                        property: "border.color"
                        duration: 200
                    }
                }
            }

            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation {
                        duration: 0
                    }

                    PropertyAnimation {
                        target: gradientStop
                        property: "color"
                        duration: 200
                    }
                }
            }
            to: "unchecked"
            from: "down,cheked"
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:8;height:34;width:34}D{i:4}D{i:2}D{i:12;transitionDuration:2000}
D{i:35;transitionDuration:2000}D{i:58;transitionDuration:2000}D{i:81;transitionDuration:2000}
D{i:104;transitionDuration:2000}
}
##^##*/

