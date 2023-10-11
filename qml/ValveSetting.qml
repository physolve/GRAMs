import QtQuick

Rectangle {
    id: valveSetting
    color: "black"
    width: 200
    height: 200
    function changeTextFront(str){
        customPlus.text = str
    }
    //--> slide
    Flipable {
        id: flipable
        anchors.centerIn: parent
        property bool flipped: false
        front: Rectangle {
            id: customPlus
            implicitWidth: 120
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
            implicitWidth: 120
            implicitHeight: 60
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
            property string text: "test back"
            Text{
                id: customBackText
                anchors.fill: parent
                font.pixelSize: 16
                text: customBack.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
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
        onClicked: flipable.flipped = !flipable.flipped
    }
}