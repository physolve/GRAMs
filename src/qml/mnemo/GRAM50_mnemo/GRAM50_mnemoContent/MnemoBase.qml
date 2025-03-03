import QtQuick
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0

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
        source: "background/GRAMsMimicNew-beta.png"
        // fillMode: Image.PreserveAspectFit
        width: 1380
        height: 960
        fillMode: Image.PreserveAspectCrop
        smooth: true
        ValveButton{
            id: k_AR1
            x: 115
            y: 425
            checked: Grams.vAR1State
            onClicked: Grams.setValveState(checked, 0) // or name
        }

        ValveButton {
            id: k_AR2
            x: 189
            y: 425
            checked: Grams.vAR2State
            onClicked: Grams.setValveState(checked, 1) // or name
        }

        ValveButton {
            id: k_AR3
            x: 261
            y: 425
            checked: Grams.vAR3State
            onClicked: Grams.setValveState(checked, 2) // or name
        }

        ValveButton {
            id: k_AR4
            x: 427
            y: 171
            checked: Grams.vAR4State
            onClicked: Grams.setValveState(checked, 3) // or name
        }
        ValveButton {
            id: k_AR5
            x: 427
            y: 425
            checked: Grams.vAR5State
            onClicked: Grams.setValveState(checked, 4) // or name
        }
        ValveButton {
            id: k_AR6
            x: 427
            y: 637
            checked: Grams.vAR6State
            onClicked: Grams.setValveState(checked, 6) // or name
        }
        ValveButton {
            id: k_S1
            x: 315
            y: 308
            checked: Grams.vS1State
            onClicked: Grams.setValveState(checked, 9) // or name
        }

        ValveButton {
            id: k_S2
            x: 315
            y: 212
            checked: Grams.vS2State
            onClicked: Grams.setValveState(checked, 10) // or name
        }
        ValveButton {
            id: k_S3
            x: 315
            y: 109
            checked: Grams.vS3State
            onClicked: Grams.setValveState(checked, 11) // or name
        }
        ValveButton {
            id: k_S4
            x: 618
            y: 267
            checked: Grams.vS4State
            onClicked: Grams.setValveState(checked, 8) // or name
        }

        ValveButton {
            id: k_R1
            x: 818
            y: 365
            checked: Grams.vR1State
            onClicked: Grams.setValveState(checked, 12) // or name
        }

        ValveButton {
            id: k_R2
            x: 818
            y: 425
            checked: Grams.vR2State
            onClicked: Grams.setValveState(checked, 13) // or name
        }

        ValveButton {
            id: k_R3
            x: 819
            y: 487
            checked: Grams.vR3State
            onClicked: Grams.setValveState(checked, 14) // or name
        }

        ValveButton {
            id: k_R4
            x: 995
            y: 308
            checked: Grams.vR4State
            onClicked: Grams.setValveState(checked, 15) // or name
        }

        ValveButton {
            id: k_R5
            x: 1106
            y: 366
            checked: Grams.vR5State
            onClicked: checked ? console.log("Chamber open") : console.log("Chamber close")
        }

        ValveButton {
            id: k_SL1
            x: 819
            y: 592
            checked: Grams.vSL1State
            onClicked: Grams.setValveState(checked, 7) // or name
        }

        ValveButton {
            id: k_SL2
            x: 514
            y: 592
            checked: Grams.vSL2State
            onClicked: Grams.setValveState(checked, 5) // or name
        }

        SensorWidget {
            id: s_SH
            x: 479
            y: 242
            value: Grams.guiValsPres.prSH
        }

        SensorWidget {
            id: s_SA
            x: 592
            y: 147
            value: Grams.guiValsPres.prSA
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
            value: Grams.guiValsPres.prSK
        }

        TempWidget {
            id: t_ST
            x: 545
            y: 417
            value: Grams.guiValsPres.tmS
        }

        SensorWidget {
            id: s_RH
            x: 890
            y: 241
            value: Grams.guiValsPres.prRH
        }

        SensorWidget {
            id: s_RA
            x: 969
            y: 147
            value: Grams.guiValsPres.prRA
        }

        SensorWidget {
            id: s_RL
            x: 1070
            y: 147
            value: Grams.guiValsPres.prRL
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
