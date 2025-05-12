import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grams.backendSourceSingleton 1.0
import Grams.testFieldSingleton 1.0

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
            id: mARA
            x: 5
            y: -lumber.width/32+5
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 20
                y: parent.height/2 - 20
                width: 40
                height: 40
                radius: 20
                border.color: "#464646";
            }
        }
        Rectangle{
            id: mSC1
            x: 5
            y: 2*height + 5 
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 25
                y: parent.height/2 - 25
                width: 50
                height: 50
                radius: 25
                border.color: "#464646";
            }
        }
        Rectangle{
            id: mSC2
            x: 5
            y: 3*height + 10
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 25
                y: parent.height/2 - 25
                width: 50
                height: 50
                radius: 25
                border.color: "#464646";
            }
        }
        Rectangle{
            id: mSC3
            x: 5
            y: 4*height + 15
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 25
                y: parent.height/2 - 25
                width: 50
                height: 50
                radius: 25
                border.color: "#464646";
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
            x: -lumber.width/16
            y: 2*height + 5 
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 20
                y: parent.height/2 - 20
                width: 40
                height: 40
                radius: 20
                border.color: "#464646";
            }
        }
        Rectangle{
            id: mRR2
            x: -lumber.width/16
            y: 3*height + 10
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 20
                y: parent.height/2 - 20
                width: 40
                height: 40
                radius: 20
                border.color: "#464646";
            }
        }
        Rectangle{
            id: mRR3
            x: -lumber.width/16
            y: 4*height + 15
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 20
                y: parent.height/2 - 20
                width: 40
                height: 40
                radius: 20
                border.color: "#464646";
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
                x: -20
                y: height-20
                height: 80
                width: lumber.width/16
                color:"transparent"; border.color: "#464646";
                Rectangle{
                    y: parent.height/2 - 20
                    width: 40
                    height: 40
                    radius: 20
                    border.color: "#464646";
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
            id: mARG1
            x: 5
            y: 5
            height: 100
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 25
                y: parent.height/2 - 25
                width: 50
                height: 50
                radius: 25
                border.color: "#464646";
            }
        }
        Rectangle{
            id: mARG2
            x: 5
            y: height + 10
            height: 100
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 25
                y: parent.height/2 - 25
                width: 50
                height: 50
                radius: 25
                border.color: "#464646";
            }
        }
        Rectangle{
            id: mARG3
            x: 5
            y: 2*height + 15
            height: 100
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width - 25
                y: parent.height/2 - 25
                width: 50
                height: 50
                radius: 25
                border.color: "#464646";
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
            // x: lumber.width/32
            y: -20
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width/2 - 20
                // y: parent.height/2 - 20
                width: 40
                height: 40
                radius: 20
                border.color: "#464646";
            }
        }
        Rectangle{
            // x: lumber.width/32
            y: height-20
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                x: parent.width/2 - 20
                // y: parent.height/2 - 20
                width: 40
                height: 40
                radius: 20
                border.color: "#464646";
            }
        }
        Rectangle{
            x: parent.width - 40
            y: height-20
            height: 80
            width: lumber.width/16
            color:"transparent"; border.color: "#464646";
            Rectangle{
                y: parent.height/2 - 20
                width: 40
                height: 40
                radius: 20
                border.color: "#464646";
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
        Button{
            id: connParamsBtn
            x: 5
            y: 5
            width: 150
            height: 50
            text: "Открыть параметры"
            onClicked: lumber.connParams()
        }
    }
}