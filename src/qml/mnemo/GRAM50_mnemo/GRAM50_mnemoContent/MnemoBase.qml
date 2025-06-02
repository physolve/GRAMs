import QtQuick
import QtQuick.Controls
import Grams.backendSourceSingleton 1.0
import Grams.valveControlSingleton 1.0

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
            checked: ValveControl.guiValve["AR1"]
            onClicked: ValveControl.setValveState(checked, 0) // or name
        }

        ValveButton {
            id: k_AR2
            x: 189
            y: 425
            checked: ValveControl.guiValve["AR2"]
            onClicked: ValveControl.setValveState(checked, 1) // or name
        }

        ValveButton {
            id: k_AR3
            x: 261
            y: 425
            checked: ValveControl.guiValve["AR3"]
            onClicked: ValveControl.setValveState(checked, 2) // or name
        }

        ValveButton {
            id: k_AR4
            x: 427
            y: 171
            checked: ValveControl.guiValve["AR4"]
            onClicked: ValveControl.setValveState(checked, 3) // or name
        }
        ValveButton {
            id: k_AR5
            x: 427
            y: 425
            checked: ValveControl.guiValve["AR5"]
            onClicked: ValveControl.setValveState(checked, 4) // or name
        }
        ValveButton {
            id: k_AR6
            x: 427
            y: 637
            checked: ValveControl.guiValve["AR6"]
            onClicked: ValveControl.setValveState(checked, 6) // or name
        }
        ValveButton {
            id: k_S1
            x: 315
            y: 308
            checked: ValveControl.guiValve["S1"]
            onClicked: ValveControl.setValveState(checked, 9) // or name
        }

        ValveButton {
            id: k_S2
            x: 315
            y: 212
            checked: ValveControl.guiValve["S2"]
            onClicked: ValveControl.setValveState(checked, 10) // or name
        }
        ValveButton {
            id: k_S3
            x: 315
            y: 109
            checked: ValveControl.guiValve["S3"]
            onClicked: ValveControl.setValveState(checked, 11) // or name
        }
        ValveButton {
            id: k_S4
            x: 618
            y: 267
            checked: ValveControl.guiValve["S4"]
            onClicked: ValveControl.setValveState(checked, 8) // or name
        }

        ValveButton {
            id: k_R1
            x: 818
            y: 365
            checked: ValveControl.guiValve["R1"]
            onClicked: ValveControl.setValveState(checked, 12) // or name
        }

        ValveButton {
            id: k_R2
            x: 818
            y: 425
            checked: ValveControl.guiValve["R2"]
            onClicked: ValveControl.setValveState(checked, 13) // or name
        }

        ValveButton {
            id: k_R3
            x: 819
            y: 487
            checked: ValveControl.guiValve["R3"]
            onClicked: ValveControl.setValveState(checked, 14) // or name
        }

        ValveButton {
            id: k_R4
            x: 995
            y: 308
            checked: ValveControl.guiValve["R4"]
            onClicked: ValveControl.setValveState(checked, 15) // or name
        }

        ValveButton {
            id: k_R5
            x: 1106
            y: 366
            checked: ValveControl.guiValve["R5"]
            onClicked: {
                checked ? console.log("Chamber open") : console.log("Chamber close")
                ValveControl.setManualChamberValve(checked)
            }
        }

        ValveButton {
            id: k_SL1
            x: 819
            y: 592
            checked: ValveControl.guiValve["SL1"]
            onClicked: ValveControl.setValveState(checked, 7) // or name
        }

        ValveButton {
            id: k_SL2
            x: 514
            y: 592
            checked: ValveControl.guiValve["SL2"]
            onClicked: ValveControl.setValveState(checked, 5) // or name
        }

        SensorWidget {
            id: s_SH
            x: 479
            y: 242
            value: Grams.guiPres.prSH
        }

        SensorWidget {
            id: s_SA
            x: 592
            y: 147
            value: Grams.guiPres.prSA
        }
        SensorWidget {
            id: s_SBL
            x: 162
            y: 83
            value: Grams.guiPresVirtual.prSC3
        }
        SensorWidget {
            id: s_SBS
            x: 162
            y: 189
            value: Grams.guiPresVirtual.prSC2
        }
        SensorWidget {
            id: s_SBT
            x: 162
            y: 285
            value: Grams.guiPresVirtual.prSC1
        }
        SensorWidget {
            id: s_SHM
            x: 716
            y: 145
            value: Grams.guiPres.prSK
        }

        TempWidget {
            id: t_ST
            x: 545
            y: 417
            value: Grams.guiPres.tmS // or Grams.guiTemp.tmSTube
        }

        SensorWidget {
            id: s_RH
            x: 890
            y: 241
            value: Grams.guiPres.prRH
        }

        SensorWidget {
            id: s_RA
            x: 969
            y: 147
            value: Grams.guiPres.prRA
        }

        SensorWidget {
            id: s_RL
            x: 1070
            y: 147
            value: Grams.guiPres.prRL
        }
        TempWidget {
            id: t_RT
            x: 943
            y: 417
            value: Grams.guiTemp.tmRTube
        }
        TempWidget {
            id: t_RC
            x: 1195
            y: 409
            value: Grams.guiTemp.tmF
        }

        SensorWidget {
            id: s_RC
            x: 1221
            y: 327
            value: Grams.guiPresVirtual.prRF
        }
        SensorWidget {
            id: s_ARV
            x: 481
            y: 659
            value: Grams.guiPres.prARV
        }
    }
}
