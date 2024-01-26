

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick 6.2
import QtQuick.Controls
import QtQuick.Studio.Components
import QtQuick.Shapes
import QtQuick.Layouts
import QtQuick.Studio.Effects

import ".."
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
    }
    Image {
        id: image
        visible: true
        anchors.fill: parent
        source: "images/GRAMsMimicNew.svg"
        anchors.rightMargin: 0
        anchors.bottomMargin: 2
        anchors.leftMargin: 0
        anchors.topMargin: -2
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
            id: v_k110
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
    PressureSlider {
        id: p_dd312
        x: 563
        y: 142
    }
    PressureSlider {
        id: p_dd311
        x: 563
        y: 228
    }

    PressureSlider {
        id: p_dd331
        x: 916
        y: 269
    }

    PressureSlider {
        id: p_dd332
        x: 916
        y: 142
    }
    PressureSlider {
        id: p_dd334
        x: 916
        y: 206
    }
    PressureSlider {
        id: p_dv301
        x: 466
        y: 605
    }

    TempSlider {
        id: t_dt359
        x: 1108
        y: 376
    }

    TempSlider {
        id: t_dt358
        x: 916
        y: 376
    }
    TempSlider {
        id: t_dt360
        x: 740
        y: 182
    }
}

/*##^##
Designer {
    D{i:0}D{i:1;locked:true}D{i:2;locked:true}D{i:3;locked:true}
}
##^##*/

