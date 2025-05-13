import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.backendSourceSingleton 1.0
import Grams.testFieldSingleton 1.0
import "content"
Item {
    id: lumber
    signal pickChamber()
    signal connParams()
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
            visible: false
            x: 5
            y: -lumber.width/32+5
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 20
                y: parent.height/2 - 20
            }
            VirtualFlow{
                anchors.centerIn: parent
                name: "Сброс"
                value: Infinity
            }
        }
        Rectangle{
            id: mSC1
            visible: false
            x: 5
            y: 2*height + 5 
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 25
                y: parent.height/2 - 25
            }
            VirtualPressure{
                anchors.centerIn: parent
                name: "большая"
            }
        }
        Rectangle{
            id: mSC2
            visible: false
            x: 5
            y: 3*height + 10
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 25
                y: parent.height/2 - 25
            }
            VirtualPressure{
                anchors.centerIn: parent
                name: "средняя"
            }
        }
        Rectangle{
            id: mSC3
            visible: false
            x: 5
            y: 4*height + 15
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 25
                y: parent.height/2 - 25
            }
            VirtualPressure{
                anchors.centerIn: parent
                name: "маленькая"
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
            id: mRR1
            visible: false
            x: -lumber.width/16
            y: 2*height + 5 
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 20
                y: parent.height/2 - 20
            }
            VirtualFlow{
                anchors.centerIn: parent
                name: "быстрый"
                value: Infinity
            }
        }
        Rectangle{
            id: mRR2
            visible: false
            x: -lumber.width/16
            y: 3*height + 10
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 20
                y: parent.height/2 - 20
            }
            VirtualFlow{
                anchors.centerIn: parent
                name: "средний"
                value: Infinity
            }
        }
        Rectangle{
            id: mRR3
            visible: false
            x: -lumber.width/16
            y: 4*height + 15
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 20
                y: parent.height/2 - 20
            }
            VirtualFlow{
                anchors.centerIn: parent
                name: ""
                value: Infinity
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
            }
            Rectangle{
                visible: true
                x: -20
                y: height-20
                height: 80
                width: 2*lumber.width/16
                color:"transparent"; border.color: "#464646";
                ValveIndicator{
                    y: parent.height/2 - 20
                }
                VirtualPressure{
                    anchors.centerIn: parent
                    name: "камера"
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
            }
        }
        Rectangle{
            id: mARG1
            visible: false
            x: 5
            y: 5
            height: 100
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 25
                y: parent.height/2 - 25
            }
            VirtualFlow{
                anchors.centerIn: parent
                name: "Газ 1"
                value: Infinity
            }
        }
        Rectangle{
            id: mARG2
            visible: false
            x: 5
            y: height + 10
            height: 100
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 25
                y: parent.height/2 - 25
            }
            VirtualFlow{
                anchors.centerIn: parent
                name: "Газ 2"
                value: Infinity
            }
        }
        Rectangle{
            id: mARG3
            visible: false
            x: 5
            y: 2*height + 15
            height: 100
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                x: parent.width - 25
                y: parent.height/2 - 25
            }
            VirtualFlow{
                anchors.centerIn: parent
                name: "Газ 3"
                value: Infinity
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
            visible: false
            x: lumber.width/32
            y: 5
            height: 150
            width: parent.width - lumber.width/32
            color:"transparent"; border.color: "#464646";
            VirtualPressure{
                anchors.centerIn: parent
                name: "бочка"
            }
        }
        Rectangle{
            // x: lumber.width/32
            visible: false
            y: -20
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualFlow{
                anchors.centerIn: parent
                name: ""
                value: Infinity
            }
            ValveIndicator{
                x: parent.width/2 - 20
                y: -10
            }
        }
        Rectangle{
            // x: lumber.width/32
            visible: false
            y: height-20
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            VirtualFlow{
                anchors.centerIn: parent
                name: ""
                value: Infinity
            }
            ValveIndicator{
                x: parent.width/2 - 20
                y: -10
            }
            
        }
        Rectangle{
            visible: false
            x: parent.width - 40
            y: height-20
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            ValveIndicator{
                y: parent.height/2 - 20
            }
        }
        Rectangle{
            x: 5
            y: 150
            height: 175
            width: parent.width - 10
            color:"transparent"; border.color: "#464646";
            LVacuum{
                anchors.centerIn: parent
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
        LTools{
            anchors.centerIn: parent
        }
    }
    Button{
        id: connParamsBtn
        x: parent.width-150
        y: 5
        width: 150
        height: 50
        text: "Открыть настройки"
        onClicked: lumber.connParams()
    }
}