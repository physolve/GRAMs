import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Rectangle {
    //id: visualChannelMapping
    //first row is channels
    //second row is sensors with arrows to left and right transposition
    
    width: 640
    height: 320

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#f6f6f6" }
        GradientStop { position: 0.9; color: "#d7d7d7" }
        GradientStop { position: 1.0; color: "#282828" } //
    }
    
    //property var namesList: undefined

    function setNameList(namesList){
        console.log("in setNameList VCM " + JSON.stringify(namesList))
        for(const [key, value] of Object.entries(namesList)){
            namesModel.append({"name": key, "curColor": "#53d769"})
            channelsModel.append({"num": value, "curColor": "#388049"})
        }
    }

    function getMappedNames(){
        let result = []
            for(let i = 0; i < channelsModel.count; i++){ // in order of channels
                result.push(namesModel.get(i).name)           
            }
        return result
    }
    // ObjectModel {
    //     id: namesModel
    // }
    ListModel{
        id: namesModel
    }

    Component{
        id: sensorName
        Rectangle { 
            height: 40; width: 130; color: curColor 
            //required property int index
            property int idx: 0
            Text { anchors.centerIn: parent; color: "black"; text: "Sensor name " + name; font.pointSize:12 }
            Layout.alignment: Qt.AlignCenter
            Rectangle{
                height: 40; width: 20; color: "#7dbbffFF"
                anchors.left: parent.left
                anchors.top: parent.top
                MouseArea{
                    anchors.fill: parent
                    onClicked: index>0 ? namesModel.move(index,index-1,1) : console.log("leftest")
                }
            }
            Rectangle{
                height: 40; width: 20; color: "#7dbbffFF"
                anchors.right: parent.right
                anchors.top: parent.top
                MouseArea{
                    anchors.fill: parent
                    onClicked: index<namesModel.count-1 ? namesModel.move(index,index+1,1) : console.log("rightest")
                }
            }
        }
    }
    
    ListView{
        id: namesGrid
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        orientation: ListView.Horizontal
        //width: namesModel.width+10
        interactive: false
        spacing: 10
        height: 40
        anchors.margins: 10
        model: namesModel
        delegate: sensorName
    }

    ListModel{
        id: channelsModel
    }
    
    Component{
        id: channelName
        Rectangle { 
            height: 40; width: 130; color: curColor 
            //required property int index
            property int idx: 0 // get this Property for mapping (or index)
            Text { anchors.centerIn: parent; color: "white"; text: "Channel num " + num; font.pointSize:12 }
            Layout.alignment: Qt.AlignCenter
            // channels are not movable
            // Rectangle{
            //     height: 40; width: 20; color: "#7dbbffFF"
            //     anchors.left: parent.left
            //     anchors.top: parent.top
            //     MouseArea{
            //         anchors.fill: parent
            //         onClicked: index>0 ? channelsModel.move(index,index-1,1) : console.log("leftest")
            //     }
            // }
            // Rectangle{
            //     height: 40; width: 20; color: "#7dbbffFF"
            //     anchors.right: parent.right
            //     anchors.top: parent.top
            //     MouseArea{
            //         anchors.fill: parent
            //         onClicked: index<channelsModel.count-1 ? channelsModel.move(index,index+1,1) : console.log("rightest")
            //     }
            // }
        }
    }

    ListView{
        id: channelsGrid
        anchors.top: namesGrid.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        orientation: ListView.Horizontal
        //width: namesModel.width+10
        interactive: false
        spacing: 10
        height: 40
        anchors.margins: 10
        model: channelsModel
        delegate: channelName
    }

    Button{
        id: exportChannelMapping
        anchors.top: channelsGrid.bottom
        anchors.left: parent.left
        anchors.margins: 10
        text: "Save mapping"
        width: 60
        onClicked: {
            let result = []
            for(let i = 0; i < channelsModel.count; i++){ // in order of channels
                result.push(namesModel.get(i).name)           
            }
            console.log(result)
        }
    }

    Component.onCompleted: {
        console.log("Visual Channel Mapping completed")
        //let alphabet = ["a","b","c","d"]
        // for(let i = 0; i<4; i++){
        //     //let curName = sensorName.createObject()
        //     //curName.idx = i
        //     namesModel.append({"name": i+alphabet[i], "curColor": "#53d769"})
        //     channelsModel.append({"num": i+alphabet[alphabet.length-1-i], "curColor": "#388049"})
        // }
    }
    /*

    */
    
    // GridLayout{
    //     //anchors.fill: parent
    //     id: namesGrid
    //     anchors.top: parent.top
    //     anchors.left: parent.left
    //     width: repHere.width+10
    //     height: 40
    //     anchors.margins: 10
    //     rowSpacing: 4
    //     columnSpacing: 4
    //     flow: GridLayout.LeftToRight
    //     columns: repHere.count
    //     Repeater{
    //     }
        
    // }
    // GridLayout {
    //     //columns: implicitW < parent.width ? -1 : parent.width / columnImplicitWidth
    //     rowSpacing: 4
    //     columnSpacing: 4
    //     flow: GridLayout.LeftToRight
    //     columns: 3
    //     rows: 1//itemModel.count%3+1
    //     //property int columnImplicitWidth: children[0].implicitWidth + columnSpacing
    //     //property int implicitW: itemModel.count * columnImplicitWidth
    //     ObjectModel { // fill using profile + profileList
    //         id: namesModel
    //     }
    //     Repeater { 
    //         model: namesModel
    //     }
    // }
    /*
    id: repHere
            model: arrayS
            Rectangle { 
                height: 40; width: 130; color: "#53d769" 
                property int idx: 0
                required property string modelData
                required property int index
                required property var model
                Text { anchors.centerIn: parent; color: "black"; text: "Sensor name " + modelData; font.pointSize:14 } //+ " " + index
                Layout.alignment: Qt.AlignCenter
                Rectangle{
                    height: 40; width: 20; color: "#7dbbffFF"
                    anchors.left: parent.left
                    anchors.top: parent.top
                    MouseArea{
                        anchors.fill: parent
                        onClicked: {
                            if(index > 0){
                                const temp = model[index].value
                                model[index].value = model[index-1].value
                                model[index-1].value = temp
                            }
                            else console.log(model + " " + arrayS)
                        }
                        //index>0 ? [arrayS[index-1], arrayS[index]] = [arrayS[index], arrayS[index-1]] : console.log("leftest")
                    }
                }
                Rectangle{
                    height: 40; width: 20; color: "#7dbbffFF"
                    anchors.right: parent.right
                    anchors.top: parent.top
                    MouseArea{
                        anchors.fill: parent
                        onClicked: index<model.count-1 ? [arrayS[index], arrayS[index+1]] = [arrayS[index+1], arrayS[index]] : console.log("rightest")
                    }
                }
            }
    */


}