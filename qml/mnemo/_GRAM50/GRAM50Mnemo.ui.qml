

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
    id: item1 //?
    width: rectangle.width
    height: rectangle.height
    function setPresValues(presValues) {
        //presValues = values
        //refreshPresValues()
        s_311.value = presValues.DD311
        s_312.value = presValues.DD312
        s_331.value = presValues.DD331
        s_332.value = presValues.DD332
        s_334.value = presValues.DD334
        s_341.value = presValues.DD341
        t_341.value = presValues.DT341
        t_314.value = presValues.DT314

    }

    function setTempValues(tempValues){
        t_352.value = tempValues.DT352
        t_354.value = tempValues.DT354
        t_356.value = tempValues.DT356
        t_358.value = tempValues.DT358
        t_359.value = tempValues.DT359

    }

    function setValves(valveStates){
        v_k104.checked = valveStates.v_k104
        v_k109.checked = valveStates.v_k109
        v_k114.checked = valveStates.v_k114
        v_k118.checked = valveStates.v_k118
        v_k131.checked = valveStates.v_k131
        v_k133.checked = valveStates.v_k133
        v_k135.checked = valveStates.v_k135
        v_k151.checked = valveStates.v_k151
        v_k153.checked = valveStates.v_k153
        v_k155.checked = valveStates.v_k155
        v_k171.checked = valveStates.v_k171
        v_k173.checked = valveStates.v_k173
        v_k176.checked = valveStates.v_k176
        v_k178.checked = valveStates.v_k178
        v_k179.checked = valveStates.v_k179
        v_k192.checked = valveStates.v_k192
    }

    Connections {
        target: _myModel
        onDataChanged: {
            setPresValues(_myModel.getCurPressureValues()) //gRAMsMnemoForm  it's Working fine // NOW IT RERTURNS QVariantMap
            setTempValues(_myModel.getCurTempValues())
        }
    }
    Connections {
        target: _valveModel
        onDataChanged: {
            setValves(_valveModel.getCurStates())
        }
    }
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
        source: "images/GRAMsMimicNew.svg"
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
        anchors.margins: 0
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
            y: 401
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
            y: 208
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
            y: 208
            width: 34
            height: 34
            checkable: true
            checked: true
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
            y: 549
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k179
            x: 492
            y: 547
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k176
            x: 421
            y: 587
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k114
            x: 273
            y: 402
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k109
            x: 205
            y: 402
            width: 34
            height: 34
            checkable: true
        }

        Valve {
            id: v_k104
            x: 144
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
            y: 115
            width: 34
            height: 34
            checkable: true
        }
    }
    SensorWidget {
        id: s_C3
        x: 156
        y: 112
        value: 0.000097
    }

    SensorWidget {
        id: s_C2
        x: 156
        y: 205
        value: 0.0001
    }

    SensorWidget {
        id: s_C1
        x: 156
        y: 291
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
        x: 461
        y: 625
        value: 0.0001
    }

    SensorWidget {
        id: s_341
        x: 726
        y: 205
        value: 0.0001
    }

    ThermoWidget {
        id: t_341
        x: 740
        y: 164
        value: 15
    }

    ThermoWidget {
        id: t_314
        x: 528
        y: 395
        value: 15
    }

    ThermoWidget {
        id: t_352
        x: 41
        y: 112
        value: 15
    }

    ThermoWidget {
        id: t_354
        x: 41
        y: 205
        value: 15
    }

    ThermoWidget {
        id: t_356
        x: 41
        y: 288
        value: 15
    }

    ThermoWidget {
        id: t_358
        x: 896
        y: 395
        value: 15
    }

    ThermoWidget {
        id: t_359
        x: 1083
        y: 384
        value: 15
    }
}
/*##^##
Designer {
    D{i:0}D{i:1;locked:true}D{i:2;locked:true}D{i:29}
}
##^##*/

