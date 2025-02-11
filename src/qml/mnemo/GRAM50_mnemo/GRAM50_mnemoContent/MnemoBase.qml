import QtQuick
import QtQuick.Controls

Item {
    width: gRAMsMimicNew.width
    height: gRAMsMimicNew.height

    // color: Constants.backgroundColor
    Component.onCompleted:{
        test()
    }

    function test(){
        console.log("print")
    }

    Image {
        id: gRAMsMimicNew
        source: "background/GRAMsMimicNew-alpha.png"
        // fillMode: Image.PreserveAspectFit
        width: 1380
        height: 960
        fillMode: Image.PreserveAspectCrop
        smooth: true
        ValveButton{
            id: k_AR1
            x: 115
            y: 425
        }

        ValveButton {
            id: k_AR2
            x: 189
            y: 425
        }

        ValveButton {
            id: k_AR3
            x: 261
            y: 425
        }

        ValveButton {
            id: k_AR4
            x: 427
            y: 171
        }
        ValveButton {
            id: k_AR5
            x: 427
            y: 425
        }
        ValveButton {
            id: k_AR6
            x: 427
            y: 637
        }
        ValveButton {
            id: k_S1
            x: 315
            y: 308
        }

        ValveButton {
            id: k_S2
            x: 315
            y: 212
        }
        ValveButton {
            id: k_S3
            x: 315
            y: 109
        }
        ValveButton {
            id: k_S4
            x: 618
            y: 267
        }

        ValveButton {
            id: k_R1
            x: 818
            y: 365
        }

        ValveButton {
            id: k_R2
            x: 818
            y: 425
        }

        ValveButton {
            id: k_R3
            x: 819
            y: 487
        }

        ValveButton {
            id: k_R4
            x: 995
            y: 308
        }

        ValveButton {
            id: k_R5
            x: 1106
            y: 366
        }

        ValveButton {
            id: k_SL1
            x: 743
            y: 592
        }

        SensorWidget {
            id: s_SH
            x: 479
            y: 242
        }

        SensorWidget {
            id: s_SA
            x: 592
            y: 147
        }
        SensorWidget {
            id: s_SBS
            x: 162
            y: 189
        }
        SensorWidget {
            id: s_SBL
            x: 162
            y: 83
        }
        SensorWidget {
            id: s_SBT
            x: 162
            y: 285
        }
        SensorWidget {
            id: s_SHM
            x: 716
            y: 145
        }

        TempWidget {
            id: t_ST
            x: 545
            y: 417
        }

        SensorWidget {
            id: s_RH
            x: 890
            y: 241
        }

        SensorWidget {
            id: s_RA
            x: 969
            y: 147
        }

        SensorWidget {
            id: s_RL
            x: 1070
            y: 147
        }
        TempWidget {
            id: t_RT
            x: 943
            y: 417
        }
        TempWidget {
            id: t_RC
            x: 1195
            y: 409
        }

        SensorWidget {
            id: s_RC
            x: 1221
            y: 327
        }
        SensorWidget {
            id: s_ARV
            x: 481
            y: 659
        }
    }
}
