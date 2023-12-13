import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Rectangle {
    //id: visualChannelMapping
    //first row is channels
    //second row is sensors with arrows to left and right transposition
    
    width: 740
    height: 320
    border.color: "black"; color: "transparent"
    /*
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#f6f6f6" }
        GradientStop { position: 0.9; color: "#d7d7d7" }
        GradientStop { position: 1.0; color: "#282828" } //
    }
    */
    //property var namesList: undefined

    function setNameList(namesList){
        //console.log("in setNameList VCM " + JSON.stringify(namesList))
        const sortedObject = namesList.sort((a, b) => a.cch - b.cch)
        //console.log("SORTED " + JSON.stringify(sortedObject))
        let result = sortedObject.map(a=>a.name)
        // for(const [key, value] of Object.entries(sortedObject)){
        //     namesModel.append({"name": key, "curColor": "#53d769"})
        //     channelsModel.append({"num": value, "curColor": "#388049"})
        // }
        for(const element of result){
            namesModel.append({"name": element, "curColor": "#53d769"})
        }
    }
    function setChannelList(channelList){
        // for (let i = 0; i < channelList; i++){
        //     channelsModel.append({"num": i, "curColor": "#388049"})
        // }
        channelsModel = channelList
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
            height: 40; width: 60; color: curColor 
            Text { anchors.centerIn: parent; color: "black"; text: name; font.pointSize:12 }
            Rectangle{
                height: 15; width: 15; color: "#7dbbffFF"
                radius: 5
                anchors.left: parent.left
                //anchors.top: parent.top
                anchors.verticalCenter: parent.verticalCenter
                MouseArea{
                    anchors.fill: parent
                    onClicked: index>0 ? namesModel.move(index,index-1,1) : console.log("leftest")
                }
            }
            Rectangle{
                height: 15; width: 15; color: "#7dbbffFF"
                radius: 5
                anchors.right: parent.right
                //anchors.top: parent.top
                anchors.verticalCenter: parent.verticalCenter
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

    // ListModel{
    //     id: channelsModel
    // }
    
    Component{
        id: channelName
        Rectangle { 
            height: 40; width: 60; color: "#388049"
            //required property int index
            //property int idx: 0 // get this Property for mapping (or index)
            Text { anchors.centerIn: parent; color: "white"; text: index; font.pointSize:12 }
        }
    }

    property int channelsModel: 0

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
        width: 180
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
}