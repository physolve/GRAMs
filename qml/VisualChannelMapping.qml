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
        console.log("I'm setting namesList (b)")
        //console.log("in setNameList VCM " + JSON.stringify(namesList))
        const sortedObject = namesList.sort((a, b) => a.cch - b.cch)
        //console.log("SORTED " + JSON.stringify(sortedObject))
        let result = sortedObject.map(a=>a.name)

        let colors = ["#fac89e","#e3e891","#c2fc99","#a3fcb3","#92e8d5","#96c8f2","#ada8ff","#ce94f7","#ed94dd","#fea8bb"]
        namesModel.clear()
        for(const [index, element] of result.entries()){
            namesModel.append({"name": element, "curColor": colors[index]})
        }
    }
    function setChannelList(channelList){
        console.log("I'm setting channelList (a) " + channelList)
        channelsModel = channelList
        
        // here is matching virtual sensors array and profile
        
        if(namesModel.count <= channelsModel){
            let unnamedCount = channelsModel-namesModel.count 
            while(unnamedCount--){
                namesModel.append({"name": "unnamed", "curColor": "#b5bec4"})
            }
        }
        else {
            let extraCount = namesModel.count-channelsModel
            let i = 0 
            // otherwise you can just order every unnamed to the end
            while(i < namesModel.count&&extraCount>0){
                if(namesModel.get(i).name == "unnamed"){
                    namesModel.remove(i)
                    extraCount--
                    console.log("deleted " + i + ", extra count " + extraCount)
                }
                else i++
            }
            if(extraCount > 0) console.log("you have unmapped profile sensors")
        }
    
    
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
            Text { anchors.centerIn: parent; color: "black"; text: name; font.pointSize:10 }
            Rectangle{
                height: 10; width: 10;
                radius: 4
                gradient: Gradient { //The rectangle background
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0.00;
                        color: "#600000";
                    }
                    GradientStop {
                        position: 0.40;
                        color: "#300000";
                    }
                    GradientStop {
                        position: 0.50;
                        color: "transparent";
                    }
                }
                anchors.left: parent.left
                //anchors.top: parent.top
                anchors.margins: 2
                anchors.verticalCenter: parent.verticalCenter
                MouseArea{
                    anchors.fill: parent
                    onClicked: index>0 ? namesModel.move(index,index-1,1) : console.log("leftest")
                }
            }
            Rectangle{
                height: 10; width: 10;
                radius: 4
                gradient: Gradient { //The rectangle background
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 1.00;
                        color: "#600000";
                    }
                    GradientStop {
                        position: 0.60;
                        color: "#300000";
                    }
                    GradientStop {
                        position: 0.50;
                        color: "transparent";
                    }
                }
                anchors.right: parent.right
                //anchors.top: parent.top
                anchors.margins: 2
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
            console.log(mappingResult())
        }
    }
    function mappingResult(){
        let result = []
            for(let i = 0; i < channelsModel; i++){ // in order of channels
                result.push(namesModel.get(i).name)           
            }
        return result
    }

}