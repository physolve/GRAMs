import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.backendSourceSingleton 1.0
import Grams.testFieldSingleton 1.0
import "content"
Item {
    id: lumber
    signal pickChamber()
    signal playSample()
    signal playVacuum()
    signal playInlet()
    signal expSupply()
    Rectangle{
        id: mS
        x: 5
        y: lumber.width/32+5
        height: 460
        width: 3*parent.width/8 + parent.width/16  + 15
        color:"transparent"; border.color: "#464646";
        Rectangle{ // Storage quartile metrics
            id: mSBD1
            x: lumber.width/16 + 10
            y: 5 
            height: 450
            width: lumber.width/4 + lumber.width/8
            color:"transparent"; border.color: "#464646";
            LStorage{
                anchors.centerIn: parent
            }
        }
        Rectangle{
            id: mARA // relief
            // visible: false
            x: 5
            y: -lumber.width/32+5
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualFlow{
                name: "Сброс"
                value: Infinity
            }
            ValveIndicator{
                x: parent.width - 10
                // y: parent.height/2 - 20
                rotation: 45
                anchors.verticalCenter: parent.bottom
                state: Grams.vAR4State
            }
        }
        Rectangle{
            id: mSC3
            // visible: false
            x: 5
            y: 2*height + 5 
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualPressure{
                anchors.centerIn: parent
                name: "большая"
                value: Grams.guiPresVirtual.prSC3
            }
            ValveIndicator{
                x: parent.width - 10
                state: Grams.vS3State
            }
        }
        Rectangle{
            id: mSC2
            // visible: false
            x: 5
            y: 3*height + 10
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualPressure{
                anchors.centerIn: parent
                name: "средняя"
                value: Grams.guiPresVirtual.prSC2
            }
            ValveIndicator{
                x: parent.width - 10
                state: Grams.vS2State
            }
        }
        Rectangle{
            id: mSC1
            // visible: false
            x: 5
            y: 4*height + 15
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualPressure{
                anchors.centerIn: parent
                name: "малая"
                value: Grams.guiPresVirtual.prSC1
            }
            ValveIndicator{
                x: parent.width - 10
                state: Grams.vS1State
            }
        }
    }
    Rectangle{
        id: mR
        x: 3*parent.width/8 + parent.width/16 + 20
        y: lumber.width/32+5
        height: 460
        width: parent.width/2
        color:"transparent"; border.color: "#464646";
        Rectangle{ // Storage quartile metrics
            id: mSED2
            x: 5
            y: 5 
            height: 450
            width: lumber.width/4 + lumber.width/8 - 15
            color:"transparent"; border.color: "#464646";
            LReaction{
                anchors.centerIn: parent   
            }
        }
        Rectangle{
            id: mRRs
            x: -lumber.width/32
            y: 160
            height: 100
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualFlow{
                height: parent.height
                name: "BEnode"
                value: Infinity
                
            }
            ValveIndicator{
                x: 0
                anchors.verticalCenter: parent.bottom
                state: Grams.vR1State || Grams.vR2State || Grams.vR3State
            }
            ValveIndicator{
                x: parent.width/2-15
                anchors.verticalCenter: parent.bottom
                state: Grams.vR1State || Grams.vR3State                
            }
            ValveIndicator{
                x: parent.width - 30
                anchors.verticalCenter: parent.bottom
                state: Grams.vR3State
            }
        }
        Rectangle{
            id: mRF
            x: 3*lumber.width/8 - 5 
            y: 5
            height: 225
            width: parent.width/4
            color:"transparent"; border.color: "#464646";
            LChamber{
                id: lChamber
                onPickChamber: lumber.pickChamber()
                onSetSample: lumber.playSample()
            }
            Rectangle{
                visible: true
                x: 0
                y: height-20
                height: 80
                width: 2*lumber.width/16
                color:"transparent"; border.color: "#464646";
                VirtualPressure{
                    anchors.centerIn: parent
                    name: "камера"
                    value: Grams.guiPresVirtual.prRF
                }
                ValveIndicator{
                    state: Grams.vR5State
                }
            }
        }
        Rectangle{
            id: furnace
            x: 3*lumber.width/8 - 5 
            y: height + 15
            height: 220
            width: parent.width/4
            color:"transparent"; border.color: "#464646";
            LFurnace{
                anchors.centerIn: parent
            }
        }
    }

    Rectangle{
        id: mARG
        x: 5
        y: lumber.width/32 + 470
        height: 330
        width: parent.width/4
        color:"transparent"; border.color: "#464646";
        Rectangle{
            id: mARS
            x: lumber.width/16 + 10
            y: 5
            width: 3*lumber.width/16 - 15
            height: 320 
            color:"transparent"; border.color: "#464646";
            LSupply{
                anchors.centerIn: parent
                onPlayInlet: lumber.playInlet()
            }
        }
        Rectangle{
            id: mARG1
            // visible: false
            x: 5
            y: 15
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualFlow{
                name: "Газ 1"
                value: Infinity
            }
            ValveIndicator{
                x: parent.width - 10
                state: Grams.vAR1State
            }
        }
        Rectangle{
            id: mARG2
            // visible: false
            x: 5
            y: height + 15
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualFlow{
                name: "Газ 2"
                value: Infinity
            }
            ValveIndicator{
                x: parent.width - 10
                state: Grams.vAR2State
            }
        }
        Rectangle{
            id: mARG3
            // visible: false
            x: 5
            y: 2*height + 20
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualFlow{
                name: "Газ 3"
                value: Infinity
            }
            ValveIndicator{
                x: parent.width - 10
                state: Grams.vAR3State
            }
        }
    }
    Rectangle{
        id: mARV
        x: parent.width/4 + 15
        y: lumber.width/32 + 470
        height: 330
        width: parent.width/8 + parent.width/16 
        color:"transparent"; border.color: "#464646";
        Rectangle{
            x: 5
            y: -5
            height: 90
            width: parent.width/3
            color:"transparent"; border.color: "#464646";
            VirtualPressure{
                anchors.centerIn: parent
                name: "бочка"
                ValveIndicator{
                    x: parent.width/2 - 15
                    rotation: 90
                    anchors.verticalCenter: parent.top
                    state: Grams.vAR5State
                }
                ValveIndicator{
                    x: parent.width/2 - 15
                    rotation: 90
                    anchors.verticalCenter: parent.bottom
                    state: Grams.vAR6State
                }
            }
        }
        Rectangle{
            visible: true
            x: parent.width/3 + 10
            y: 5
            height: 80
            width: 2*parent.width/3 - 15
            color:"transparent"; border.color: "#464646";
            VirtualPressure{
                anchors.centerIn: parent
                name: "масс-спект"
            }
            ValveIndicator{
                x: -15
                state: Grams.vSL2State
            }
        }
        Rectangle{
            x: 5
            y: 100
            height: 225
            width: parent.width - 10
            color:"transparent"; border.color: "#464646";
            LVacuum{
                anchors.centerIn: parent
                onPlayVacuum: lumber.playVacuum()
            }
        }
    }
    Rectangle{
        id: tools
        x: 3*parent.width/8 + parent.width/16 + 20
        y: lumber.width/32 + 470
        height: 330
        width: parent.width/2
        color:"transparent"; border.color: "#464646";
        Rectangle{
            id: expTool
            x: 5
            y: 5
            width: parent.width - 10
            height: parent.height - 10 
            color:"transparent"; border.color: "#464646";
            LTools{
                anchors.centerIn: parent
                onOpenESupply: lumber.expSupply()
            }   
        }
    }
}