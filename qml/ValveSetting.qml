import QtQuick
import QtQuick.Controls

Rectangle {
    id: valveSetting
    property bool connected: false
    property string innerName: ""
    property string innerPurpose: ""
    property string innerProfile: ""
    property var valveMap: []
    color: "black"
    width: 270
    height: 300
    function changePurposeFront(str){
        innerPurpose = str
    }
    function setDeviceLbl(str){
        innerName = str
    }
    function setDeviceProfile(str){
        innerProfile = str
    }
    function setDeviceConnected(stateBool){
        connected = stateBool
    }
    function setValvesNames(nameData){
        console.log("in setValvesNames VS " + JSON.stringify(nameData))

        valveMap = nameData
    }
    function getSettings(){
        return { inProfilePath: innerProfile }
    }
    function getMappedValves(){
        return valveMap
    }
    //--> slide
    Flipable {
        id: flipable
        anchors.centerIn: parent
        property bool flipped: false
        front: Rectangle {
            id: customPlus
            implicitWidth: 160
            implicitHeight: 60
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
            property string purpose: innerPurpose
            MouseArea {
                anchors.fill: parent
                onClicked:{
                    if(connected)
                        flipable.flipped = !flipable.flipped
                    else console.log("Device not connected: "+`${customPlus.deviceName}`)
                } 
            }
            Text{
                id: customPlusText
                anchors.fill: parent
                font.pixelSize: 16
                text: customPlus.purpose
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            anchors.centerIn: parent
        } //<-- collapse
        back: Rectangle {
            id: customBack
            implicitWidth: 250
            implicitHeight: 300
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
            property string text: "test back"
            property string deviceName: innerName
            property string deviceProfile: innerProfile
            MouseArea {
                anchors.fill: parent
                onClicked:{
                    if(connected)
                        flipable.flipped = !flipable.flipped
                    else console.log("Device not connected: "+`${customPlus.deviceName}`)
                } 
            }
            Column{
                Text{
                    id: customBackText
                    font.pixelSize: 16
                    text: customBack.text
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Label{
                    id: deviceNameLbl
                    font.pixelSize: 16
                    text: customBack.deviceName
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Label{
                    id: deviceProfileLbl
                    width: 120
                    font.pixelSize: 16
                    text: customBack.deviceProfile
                    anchors.horizontalCenter: parent.horizontalCenter
                    elide: Text.ElideRight
                }
                anchors.centerIn: parent
            }
            anchors.centerIn: parent
        } //<-- collapse

        transform: Rotation {
            axis.x: 0; axis.y: 1; axis.z: 0
            angle: flipable.flipped ? 180 : 0

            Behavior on angle {
                NumberAnimation { duration: 500 }
            }
        }
    }
    //<-- slide
}