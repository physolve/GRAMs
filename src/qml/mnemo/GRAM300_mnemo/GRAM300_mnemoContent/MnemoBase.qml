import QtQuick
import QtQuick.Controls

Item {
    width: 1380
    height: 960

    // color: Constants.backgroundColor
    Component.onCompleted:{
        test()
    }

    function test(){
        console.log("print")
    }

    Image {
        id: gRAMsMimicNew
        source: "background/GRAMsMimicNew.png"
        // fillMode: Image.PreserveAspectFit
        width: 1380
        height: 960
        fillMode: Image.PreserveAspectCrop
        smooth: true
        ValveButton{
            id: k_AR1
            x: 354
            y: 257
        }

        ValveButton {
            id: k_AR2
            x: 354
            y: 326
        }

        ValveButton {
            id: k_AR3
            x: 354
            y: 382
        }

        ValveButton {
            id: k_AR4
            x: 354
            y: 435
        }
        ValveButton {
            id: k_AR5
            x: 441
            y: 527
        }
        ValveButton {
            id: k_AR6
            x: 273
            y: 614
        }
        ValveButton {
            id: k_S1
            x: 512
            y: 503
        }

        ValveButton {
            id: k_S2
            x: 512
            y: 557
        }
        ValveButton {
            id: k_S3
            x: 556
            y: 183
        }

        ValveButton {
            id: k_R1
            x: 725
            y: 257
        }

        ValveButton {
            id: k_R2
            x: 725
            y: 326
        }

        ValveButton {
            id: k_R3
            x: 725
            y: 382
        }

        ValveButton {
            id: k_R4
            x: 915
            y: 183
        }

        ValveButton {
            id: k_R5
            x: 998
            y: 106
        }

        ValveButton {
            id: k_R6
            x: 1068
            y: 257
        }

        ValveButton {
            id: k_SL1
            x: 512
            y: 728
        }

        ValveButton {
            id: k_SL2
            x: 303
            y: 666
        }

        SensorWidget {
            id: s_SH
            x: 454
            y: 174
        }

        SensorWidget {
            id: s_SA
            x: 530
            y: 82
        }
        SensorWidget {
            id: s_SBS
            x: 555
            y: 463
        }
        SensorWidget {
            id: s_SBL
            x: 584
            y: 549
        }

        TempWidget {
            id: t_ST
            x: 532
            y: 375
        }

        SensorWidget {
            id: s_RH
            x: 800
            y: 174
        }

        SensorWidget {
            id: s_RA
            x: 888
            y: 10
        }

        SensorWidget {
            id: s_RL
            x: 1040
            y: 10
        }
        TempWidget {
            id: t_RT
            x: 863
            y: 319
        }
        TempWidget {
            id: t_RC
            x: 1205
            y: 301
        }

        SensorWidget {
            id: s_RC
            x: 1231
            y: 215
        }
        SensorWidget {
            id: s_ARV
            x: 147
            y: 528
        }

        SensorWidget {
            id: s_SLMS
            x: 247
            y: 802
        }



    }

    // AnimatedImage { id: animation; x: 932; y: 460; width: 251; height: 194;
    //     visible: false
    //     source: "background/shigure-ui-dance.gif"}
    // function makeVisible(id){
    //     if(a.checked&&c.checked&&!b.checked){
    //         animation.visible = true
    //     }
    //     else {
    //         animation.visible = false
    //     }
    // }

    // RoundButton {
    //     id: a
    //     x: 1027
    //     y: 421
    //     text: "+"
    //     checkable: true
    //     onClicked: makeVisible(1)
    // }

    // RoundButton {
    //     id: b
    //     x: 1082
    //     y: 421
    //     text: "+"
    //     checkable: true
    //     onClicked: makeVisible(2)
    // }

    // RoundButton {
    //     id: c
    //     x: 1143
    //     y: 421
    //     text: "+"
    //     checkable: true
    //     onClicked: makeVisible(3)
    // }
}
