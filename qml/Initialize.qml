import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: initialize
    visible: true
    color: "#292929"
    Material.theme: Material.Dark
    Material.accent: Material.Indigo
    //flags: Qt.Window | Qt.FramelessWindowHint
    width: 960
    height: 760
    title: qsTr("Настройки")
    Component.onCompleted: {
        profileBox.currentIndex = 0
        parseRequirements()
        realRequirements()
    }
    ColumnLayout{
        anchors.centerIn: parent
        width: parent.width
        Layout.preferredHeight:  parent.height
        visible: true
        FontLoader { id: webLoveLetter; source: "qrc:/GRAMs/fonts/LoveLetter.TTF" }
        FontLoader { id: webMomot; source: "qrc:/GRAMs/fonts/Momot___.ttf" }
        Text { text: "GRAMs"
            font.family: webMomot.font.family
            font.weight: webMomot.font.weight
            font.pixelSize: 24
            color: "white"
            horizontalAlignment: Text.AlignHCenter // ?
            Layout.alignment: Qt.AlignHCenter
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            //width: parent.width
            visible: true
            ComboBox {
                id: profileBox
                width: 100
                model: initSource.profileNames
                onActivated: {
                    parseRequirements()
                    
                }
                Layout.alignment: Qt.AlignHCenter
            }
            Label{
                id: chosenProfile
                text: profileBox.currentText
                font.family: webLoveLetter.font.family
                font.weight: webLoveLetter.font.weight
                font.pixelSize: 24
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }
        GridLayout {
            rowSpacing: 4
            columnSpacing: 4
            flow: GridLayout.LeftToRight
            columns: 3
            rows: itemModel.count%3+1
            ObjectModel {
                id: itemModel
            }
            Repeater { 
                model: itemModel
            }
        }
    }
    function parseRequirements(){
        let profileName = profileBox.currentText
        console.log("For " + profileName + " to work I need:")
        let curProfile = initSource.profileJson[profileName].controllers
        if("Advantech" in curProfile){
            console.log("\tI need to create Advantech instances")
            advantechRequest(curProfile.Advantech)
            //need function to sync with real (connected in placeholder)
        }
    }
    property var advantechAIWait: []
    property var advantechDOWait: []
    function advantechRequest(curAdvantech){ 
        console.log("Asking backend to create: ")
        for(const [key, value] of Object.entries(curAdvantech)){
            switch(value.purpose){
                case "valves":
                    console.log("\tcreate DO settings for " + value.device)
                    advantechDOWait.push(value.device)
                    advModuleDOType(value)
                    break;
                case "pressure":
                case "thermocouples":
                    console.log("\tcreate AI settings for " + value.device)
                    advantechAIWait.push(value.device)
                    advModuleAIType(value)
                    // creating sensors too!
                    break;
                default: console.log(`Sorry, we are out of ${value.purpose}.`);
            }
        }
    }
    Component{
        id: advModuleAI
        PressureSetting{
        }
    }
    Component{
        id: advModuleDO
        ValveSetting{
        }
    }
    function advModuleAIType(value){
        let profileObj = advModuleAI.createObject()
        profileObj.color = Material.color(Material.Red)
        profileObj.changePurposeFront(value.purpose)
        profileObj.setDeviceLbl(value.device) // using value[device] because it is array of obj
        if ("profile" in value){
            profileObj.setDeviceProfile(value.profile) // make somewhere profiles (maybe in resources)
        }
        itemModel.append(profileObj)
    }
    function advModuleDOType(value){
        let profileObj = advModuleDO.createObject()
        profileObj.color = Material.color(Material.Red)
        profileObj.changePurposeFront(value.purpose)
        profileObj.setDeviceLbl(value.device) // using value[device] because it is array of obj
        if ("profile" in value){
            profileObj.setDeviceProfile(value.profile) // make somewhere profiles (maybe in resources)
        }
        itemModel.append(profileObj)
    }
     function fieldModule(description){
        let realObj = advModuleAI.createObject()
        realObj.color = Material.color(Material.Yellow) 
        realObj.changePurposeFront(description) // using key because it is map (already an object)
        //realObj.setDeviceConnected(true) // find a way to decline connection
        itemModel.append(realObj)
    }
    function realRequirements(){
        let rsa = initSource.advantechDeviceMap // for now only advantech connected
        console.log("I see real devices")
        console.log(Object.values(rsa))
        console.log("\tI will compare those to profile")
        for(const [key, value] of Object.entries(rsa)){
            if(advantechAIWait.includes(value))
                console.log("\tMatch! Asking for real data to fill setting")
            else if(advantechDOWait.includes(value))
                console.log("\tMatch! Asking for real data to fill setting")
            else {
                console.log("\tNo match, creating field for " + value)
                fieldModule(value+','+key)
            }
        }
    }
    onClosing:{
        main.show()
        initialize.close()
    }
}