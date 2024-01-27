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
        profileBox.currentIndex = 1
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
                    realRequirements()
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
        itemModel.clear()
        advantechWait = []
        let profileName = profileBox.currentText
        console.log("For " + profileName + " to work I need:")
        let curProfile = initSource.profileJson[profileName].controllers
        if("Advantech" in curProfile){
            console.log("\tI need to create Advantech instances")
            advantechRequest(curProfile.Advantech)
            //need function to sync with real (connected in placeholder)
        }
    }
    property var advantechWait: []
    function advantechRequest(curAdvantech){ 
        console.log("Asking backend to create: ")
        for(const [key, value] of Object.entries(curAdvantech)){
            switch(value.purpose){
                case "valves":
                    console.log("\tcreate DO settings for " + value.device)
                    advantechWait.push({"name":value.device, "type": "DO", "index":itemModel.count })
                    advModuleDOType(value)
                    break;
                case "pressure":
                case "thermocouples":
                    console.log("\tcreate AI settings for " + value.device)
                    advantechWait.push({"name":value.device, "type": "AI", "index":itemModel.count})
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
        if("sensors" in value){
            profileObj.setMappingNames(value.sensors)
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
    function realRequirements(){
        let rsa = initSource.advantechDeviceMap // for now only advantech connected
        console.log("I see real devices")
        console.log(Object.values(rsa))
        console.log("\tI will compare those to profile")
        let waitNames = advantechWait.map(a => a.name)
        console.log(waitNames)
        for(const [key, value] of Object.entries(rsa)){
            if(waitNames.includes(value)){
                let element = advantechWait[waitNames.indexOf(value)]
                console.log("\tMatch! Asking for real data to fill setting " + value)
                switch(element.type){
                case "AI":
                    advModuleAIReal(value+','+key, element.index)
                    break
                case "DO":
                    advModuleDOReal(value+','+key, element.index)
                    break
                default: break
                }
            }  
            else {
                console.log("\tNo match, creating field for " + value)
                fieldModule(value+','+key)
            }
        }
    }
    function advModuleAIReal(description, index){
        let t_profileObj = itemModel.get(index) // take i'th item
        console.log("\tNow in "+ t_profileObj.innerName)
        t_profileObj.color = Material.color(Material.Green)
        t_profileObj.setDeviceLbl(description)
        //t_profileObj.setDeviceProfile("profile from resources") 
        t_profileObj.setDeviceConnected(true)
        console.log("set settings")

        let settings = initSource.advantechDeviceFill(description, "pressure")
        
        t_profileObj.setChannelCount(settings.channelCount)
        t_profileObj.setValueRange(settings.valueRanges)
    }
    function advModuleDOReal(description, index){
        let t_profileObj = itemModel.get(index) // take i'th item
        console.log("\tNow in "+ t_profileObj.innerName)
        t_profileObj.color = Material.color(Material.Green)
        t_profileObj.setDeviceLbl(description)
        t_profileObj.setDeviceConnected(true)
        console.log("set settings")
        let settings = initSource.advantechDeviceFill(description, "valve")
    }
    function fieldModule(description){
        let realObj = advModuleAI.createObject()
        realObj.color = Material.color(Material.Yellow) 
        realObj.changePurposeFront(description) // using key because it is map (already an object)
        //realObj.setDeviceConnected(true) // find a way to decline connection
        itemModel.append(realObj)
    }
    onClosing:{
        main.profileId = profileBox.currentIndex
        if(true){ //cfgTest[0].start
            let tabPage1 = page1.createObject(stackLayout); // работает
            container.append(tabPage1);
        }

        main.show()
        initialize.close()
    }
}
