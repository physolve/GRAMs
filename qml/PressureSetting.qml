import QtQuick
import QtQuick.Controls

Rectangle {
    id: pressureSetting
    property bool connected: false
    property string innerName: ""
    property string innerPurpose: ""
    property string innerProfile: ""
    color: "black"
    width: 270
    height: 200
    function changePurposeFront(str){
        //customPlus.purpose = str
        innerPurpose = str
    }
    function setDeviceLbl(str){
        //customBack.deviceName = str
        innerName = str
    }
    function setDeviceProfile(str){
        innerProfile = str
    }
    function setDeviceConnected(stateBool){
        connected = stateBool
    }
    function setChannelCount(cnt){
        customBack.channelCount = cnt
    }
    function setValueRange(rng){
        customBack.valueRange = rng
    }
    function getSettings(){
        // add profile path to return
        return { indexChannelStart:cmbChannelStart.currentIndex,
        indexChannelCount:cmbChannelCount.currentIndex + 1,
        indexValueRange:cmbValueRange.currentIndex,
        profilePath: innerProfile}
    }
    //--> slide
    Flipable {
        id: flipable
        anchors.centerIn: parent
        property bool flipped: false
        front: Rectangle {
            id: customPlus
            implicitWidth: 250
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
            implicitHeight: 200
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
            property string text: "test back"
            property string deviceName: innerName
            property string deviceProfile: innerProfile
            property var channelCount: 3
            property var channelStart: 0
            property var valueRange: 0
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
                ComboBox{
                    id: cmbChannelStart
                    width: 100
                    height: 40
                    font.pointSize: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    model: customBack.channelCount 
                }
                ComboBox{
                    id: cmbChannelCount
                    width: 100
                    height: 40
                    font.pointSize: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    model: customBack.channelCount
                    delegate: ItemDelegate {
                        width: parent.width
                        text: index + 1
                    }
                    displayText: Number(currentText) + 1
                    //onCurrentIndexChanged: 
                }
                ComboBox{
                    id: cmbValueRange
                    width: 180
                    height: 50
                    font.pointSize: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    model: customBack.valueRange
                    // background: Rectangle {
                    //     color: "lightgrey"
                    //     border {width: 1; color: "grey"}
                    //     implicitWidth:  50
                    //     implicitHeight: 30
                    // }
                    contentItem: Label {
                        text: cmbValueRange.currentText
                        font: cmbValueRange.font
                        wrapMode: Text.WordWrap
                        padding: 4
                        verticalAlignment: Text.AlignVCenter
                    }
                    popup: Popup {
                        y: cmbValueRange.height - 1
                        width: 200
                        implicitHeight: contentItem.implicitHeight
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: cmbValueRange.popup.visible ? cmbValueRange.delegateModel : null
                            currentIndex: cmbValueRange.highlightedIndex
                        }
                    }
                    //onCurrentIndexChanged:
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