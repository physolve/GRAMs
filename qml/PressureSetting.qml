import QtQuick
import QtQuick.Controls

Rectangle {
    id: pressureSetting
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
            implicitHeight: 200
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
            property string text: "test back"
            property string deviceName: "unknown"
            property string deviceProfile: "path"
            property var channelCount: 0
            property var channelStart: 0
            property var valueRange: 0
            Column{
                Text{
                    id: customBackText
                    //anchors.fill: parent
                    font.pixelSize: 16
                    text: customBack.text
                    anchors.horizontalCenter: parent.horizontalCenter
                    //wrapMode: Text.WordWrap
                    //verticalAlignment: Text.AlignVCenter
                }
                Text{
                    id: deviceNameLbl
                    //anchors.fill: parent
                    font.pixelSize: 16
                    text: customBack.deviceName
                    anchors.horizontalCenter: parent.horizontalCenter
                    wrapMode: Text.WordWrap
                    //verticalAlignment: Text.AlignVCenter
                }
                Text{
                    id: deviceProfileLbl
                    //anchors.fill: parent
                    width: 120
                    font.pixelSize: 16
                    text: customBack.deviceProfile
                    anchors.horizontalCenter: parent.horizontalCenter
                    elide: Text.ElideRight
                    //wrapMode: Text.WordWrap
                    //verticalAlignment: Text.AlignVCenter
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
    MouseArea {
        anchors.fill: parent
        onClicked:{
            if(flipable.connected)
                flipable.flipped = !flipable.flipped
            else console.log("Device not connected: "+`${customPlus.deviceName}`)
        } 
    }
}