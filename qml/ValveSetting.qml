import QtQuick
import QtQuick.Controls

Rectangle {
    id: valveSetting
    color: "black"
    width: 180
    height: 200
    function changeTextFront(str){
        customPlus.text = str
    }
    function setDeviceLbl(str){
        customBack.deviceName = str
    }
    function setDeviceProfile(str){
        customBack.deviceProfile = str
    }
    function setDeviceConnected(stateBool){
        flipable.connected = stateBool
    }
    //--> slide
    Flipable {
        id: flipable
        anchors.centerIn: parent
        property bool flipped: false
        property bool connected: false
        front: Rectangle {
            id: customPlus
            implicitWidth: 160
            implicitHeight: 60
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
            property string text: "test"
            Text{
                id: customPlusText
                anchors.fill: parent
                font.pixelSize: 16
                text: customPlus.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            anchors.centerIn: parent
        } //<-- collapse
        back: Rectangle {
            id: customBack
            implicitWidth: 160
            implicitHeight: 60
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
            property string text: "test back"
            property string deviceName: "unknown"
            property string deviceProfile: "path"
            Column{
                Text{
                    id: customBackText
                    anchors.fill: parent
                    font.pixelSize: 16
                    text: customBack.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                Label{
                    id: deviceNameLbl
                    anchors.fill: parent
                    font.pixelSize: 16
                    text: customBack.deviceName
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                Label{
                    id: deviceProfileLbl
                    anchors.fill: parent
                    font.pixelSize: 16
                    text: customBack.deviceProfile
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
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
    MouseArea {
        anchors.fill: parent
        onClicked:{
            if(connected)
                flipable.flipped = !flipable.flipped
            else console.log("Device not connected: "+`${customPlus.deviceName}`)
        } 
    }
}