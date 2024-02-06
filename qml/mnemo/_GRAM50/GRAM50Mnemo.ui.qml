

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick 6.2
import QtQuick.Controls
//import QtQuick.Studio.Components
//import QtQuick.Shapes
//import QtQuick.Layouts
//import QtQuick.Studio.Effects
import ".."
Item {
    id: item1
    width: rectangle.width
    height: rectangle.height
    Rectangle {
        id: rectangle
        width: 1320
        height: 900
        color: "#00685757"
    }
    Image {
        id: image
        visible: true
        anchors.fill: parent
        source: "qrc:/GRAMs/qml/mnemo/_GRAM50/images/GRAMsMimicNew.svg"
        sourceSize.height: 1340
        sourceSize.width: 972
        fillMode: Image.PreserveAspectFit
        
    }

    GroupBox {
        id: groupBox
        anchors.fill: parent
        bottomPadding: 0
        topPadding: 0
        padding: 0
        anchors.rightMargin: 0
        anchors.bottomMargin: 2
        anchors.leftMargin: 0
        anchors.topMargin: -2
        title: qsTr("GramsMnemo")
        Valve {
            id: v_k155
            x: 775
            y: 344
            width: 34
            height: 34
            checkable: true
        }
        Valve {
            id: v_k153
            x: 775
            y: 402
            width: 34
            height: 34
            checkable: true
        }
        Valve {
            id: v_k151
            x: 775
            y: 457
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k173
            x: 862
            y: 264
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k171
            x: 492
            y: 207
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k133
            x: 319
            y: 207
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k118
            x: 421
            y: 207
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k178
            x: 421
            y: 402
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k192
            x: 862
            y: 548
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k179
            x: 492
            y: 548
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k176
            x: 421
            y: 592
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k114
            x: 272
            y: 402
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k109
            x: 207
            y: 402
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k104
            x: 143
            y: 402
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k131
            x: 319
            y: 293
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k135
            x: 319
            y: 114
            width: 34
            height: 34
            checkable: true
        }
    }
    SensorWidget {
        id: s_C3
        x: 148
        y: 109
        value: 0.000097
    }

    SensorWidget {
        id: s_C2
        x: 148
        y: 202
        value: 0.0001
    }

    SensorWidget {
        id: s_C1
        x: 148
        y: 288
        value: 0.0001
    }

    SensorWidget {
        id: s_312
        x: 528
        y: 164
        value: 0.0001
    }

    SensorWidget {
        id: s_311
        x: 528
        y: 242
        value: 0.0001
    }

    SensorWidget {
        id: s_332
        x: 896
        y: 164
        value: 0.0001
    }

    SensorWidget {
        id: s_334
        x: 896
        y: 226
        value: 0.0001
    }

    SensorWidget {
        id: s_331
        x: 896
        y: 288
        value: 0.0001
    }

    SensorWidget {
        id: s_301
        x: 452
        y: 624
        value: 0.0001
    }
}

/*##^##
Designer {
    D{i:0}D{i:1;locked:true}D{i:2;locked:true}D{i:21}D{i:22}D{i:23}D{i:24}D{i:25}D{i:26}
D{i:27}D{i:28}
}
##^##*/

