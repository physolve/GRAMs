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
                model: initSource ? initSource.profileNames : null // makes me fell bad
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
            Layout.alignment: Qt.AlignHCenter
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
        Button{
            id: continueBtn
            text: qsTr("Продолжить");
            onClicked: {
                saveSettings()
                securityForm()
                exitSave()
            }
            Layout.alignment: Qt.AlignHCenter
        }
    }
    property var valveMap: ([])
    property var pressureSensors: ([])
    property var temperatureSensors: ([])

    property var m_curQuartiles: ({})
    property var m_curSecurity: ({})

    function parseRequirements(){
        itemModel.clear()
        advantechWait = []
        let profileName = profileBox.currentText
        console.log("For " + profileName + " to work I need:")
        let curProfile = initSource.profileJson[profileName]

        if("stuff" in curProfile){
            let stuff = curProfile.stuff
            if("valveMap" in stuff)
                valveMap = stuff.valveMap
            if("pressureSensors" in stuff)
                pressureSensors = stuff.pressureSensors
            if("temperatureSensors" in stuff)
                temperatureSensors = stuff.temperatureSensors
        }
        else console.log("Big, big trouble! No stuff")
        if("controllers" in curProfile){
            let curControllers = curProfile.controllers
            if("Advantech" in curControllers){
                console.log("\tI need to create Advantech instances")
                advantechRequest(curControllers.Advantech)
                //need function to sync with real (connected in placeholder)
            }
        }
        else console.log("Big, big trouble! No controllers")
        if("quartiles" in curProfile){
            let curQuartiles = curProfile.quartiles
            console.log("\t" + JSON.stringify(curQuartiles))
            // append JSON to safety in exit
            m_curQuartiles = curQuartiles
        }
        else console.log("Big, big trouble! No quartiles")
        if("security" in curProfile){
            let curSecurity = curProfile.security
            console.log("\t" + JSON.stringify(curSecurity))
            // append JSON to safety in exit
            m_curSecurity = curSecurity
        }
        else console.log("Big, big trouble! No security") 
    }
    property var advantechWait: []
    function advantechRequest(curAdvantech){ 
        console.log("Asking backend to create: ")
        for(const [key, value] of Object.entries(curAdvantech)){
            if(value.purpose == "valves"){
                console.log("\tcreate DO settings for " + value.device)
                advantechWait.push({"name":value.device, "type": "DO", "index":itemModel.count })
                value.valves = valveMap
                advModuleDOType(value)
            }
            else if (value.purpose == "pressure" || value.purpose == "temperature" ){
                console.log("\tcreate AI settings for " + value.device)
                advantechWait.push({"name":value.device, "type": "AI", "index":itemModel.count})
                if(value.purpose == "pressure")
                    value.sensors = pressureSensors
                else
                    value.sensors = temperatureSensors
                advModuleAIType(value)
            }
            else console.log(`Sorry, we are out of ${value.purpose}.`);
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
        
        profileObj.setMappingNames(value.sensors)
        profileObj.setInitialChannelCount(value.sensors.length-1)

        if("defaultType" in value){
            profileObj.setDefaultType(value.defaultType)
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
        
        profileObj.setValvesNames(value.valves)

        itemModel.append(profileObj)
    }
    function realRequirements(){
        let rsa = initSource.advantechDeviceMap // for now only advantech connected (key: BID, value: name)
        console.log("I see real devices")
        console.log(Object.values(rsa))
        console.log("\tI will compare those to profile")
        let waitNames = advantechWait.map(a => a.name)
        console.log(waitNames)
        for(const [key, value] of Object.entries(rsa)){
            if(waitNames.includes(value)){ //
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

        let settings = initSource.advantechDeviceFill(description, "sensors")
        
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
        let settings = initSource.advantechDeviceFill(description, "valves")
    }
    function fieldModule(description){
        let realObj = advModuleAI.createObject()
        realObj.color = Material.color(Material.Yellow) 
        realObj.changePurposeFront(description) // using key because it is map (already an object)
        //realObj.setDeviceConnected(true) // find a way to decline connection
        itemModel.append(realObj)
    }



    function saveSettings(){
        console.log("save controllers parameters")
        saveControllers()
    }
    function saveControllers(){
        // create index list from itemModel to separate Advantech and others
        for(let i=0; i<itemModel.count; i++){ // iterate trough itemModel
            // find AdvantechControllers index
            let lcl_item = itemModel.get(i)
            if(lcl_item.connected){ // || ?advantech
                saveAdvantechControllers(i)
                if(lcl_item.innerPurpose == "pressure" || lcl_item.innerPurpose == "temperature"){
                    console.log("create sensors model")
                    saveAdvantechSensors(i)
                }
                else if(lcl_item.innerPurpose == "valves"){
                    console.log("create valves model")
                    saveAdvantechValves(i)
                }
                else{
                    console.log("unknown controller")
                }
            }
            // add else to create SIMULATED sensors and model valves names
            else {
                let colors = ["#fac89e","#e3e891","#c2fc99","#a3fcb3","#92e8d5","#96c8f2","#ada8ff","#ce94f7","#ed94dd","#fea8bb"]
                if(lcl_item.innerPurpose == "pressure"){
                    let sortedArray = pressureSensors.sort((a, b) => a.cch - b.cch)    
                    let pressureEmul = ([])
                    for(const [index, element] of sortedArray.entries()){ // finish the sequence ->setNameList, <- mappingResult()
                        let m_parameters = {}
                        m_parameters["A"] = 1
                        m_parameters["B"] = 0
                        m_parameters["R"] = 1
                        pressureEmul.push({"name": element.name, "parameters": m_parameters, "curColor": colors[index]})
                    }
                    _myModel.appendProfileSensors(lcl_item.innerPurpose, pressureEmul)
                }
                else if(lcl_item.innerPurpose == "temperature"){
                    let sortedArray = temperatureSensors.sort((a, b) => a.cch - b.cch)    
                    let tempEmul = ([])
                    for(const [index, element] of sortedArray.entries()){ // finish the sequence ->setNameList, <- mappingResult()
                        let m_parameters = {}
                        m_parameters["A"] = 1
                        m_parameters["B"] = 0
                        m_parameters["R"] = 1
                        tempEmul.push({"name": element.name, "parameters": m_parameters, "curColor": colors[index]})
                    }
                    _myModel.appendProfileSensors(lcl_item.innerPurpose, tempEmul)
                }
                else if(lcl_item.innerPurpose == "valves"){
                    _valveModel.appendValves(valveMap)
                }
            }
        }
    }
    function saveAdvantechControllers(index){
        let t_profileObj = itemModel.get(index) // take i'th item
        let description = t_profileObj.innerName
        let purpose = t_profileObj.innerPurpose
        let settigns = t_profileObj.getSettings()
        dataSource.advantechDeviceSetting(description, purpose, settigns)
    }

    function saveAdvantechSensors(index){
        //let namesObj = {}
        let t_profileObj = itemModel.get(index);
        // add if to check connection with else kinda from profile 
        _myModel.appendProfileSensors(t_profileObj.innerPurpose, t_profileObj.getMappedNames())
    }
    function saveAdvantechValves(index){
        let t_profileObj = itemModel.get(index);
        _valveModel.appendValves(t_profileObj.getMappedValves())
    }
    // rewrite to another type of saving function

    //security parser (as values to security.cpp (safeModule))
    function securityForm(){
        if("addRemoveQuar" in m_curQuartiles === false)
            return
        let addRemove = m_curQuartiles.addRemoveQuar
        if("storageQuar" in m_curQuartiles === false)
            return
        let storage = m_curQuartiles.storageQuar
        if("reactionQuar" in m_curQuartiles === false)
            return
        let reaction = m_curQuartiles.reactionQuar
        if("secondLineQuar" in m_curQuartiles === false)
            return
        let secondLine = m_curQuartiles.secondLineQuar


        if("contradictionValves" in m_curSecurity === false) // we may be without contr valves but check with question
            return
        let contradictionRule = m_curSecurity.contradictionValves

        if("twoOfThree" in m_curSecurity === false)
            return
        let twoOfThreeRule = m_curSecurity.twoOfThree

        if("safetyQuars" in m_curSecurity === false)
            return
        let safetyQuarRules = m_curSecurity.safetyQuars
        let sPSQ = [] // [string, string, double, double]
        if("storageQuarSP" in safetyQuarRules){
            sPSQ = safetyQuarRules.storageQuarSP
            sPSQ[0] = storage[sPSQ[0]]
            // [1] = storageQuar
            sPSQ[2] = storage[sPSQ[2]]
            sPSQ[3] = storage[sPSQ[3]]
        }
        let sRSQ = [] // [string, string, double]
        if("storageQuarSR" in safetyQuarRules){
            sRSQ = safetyQuarRules.storageQuarSR
            sRSQ[0] = storage[sRSQ[0]]
            // [1] = storageQuar
            sRSQ[2] = storage[sRSQ[2]]
        }
        let sRRQ = [] // [string, string, double, double]
        if("reactionQuarSP" in safetyQuarRules){
            sRRQ = safetyQuarRules.reactionQuarSP
            sRRQ[0] = reaction[sRRQ[0]]
            // [1] = reactionQuar
            sRRQ[2] = reaction[sRRQ[2]]
            sRRQ[3] = reaction[sRRQ[3]]
        }
        
        safeModule.setContradictionValves(contradictionRule, twoOfThreeRule)
        safeModule.setRangePressureValves(sPSQ[0], sPSQ[1], sPSQ[2], sPSQ[3])
        safeModule.setSafeReleaseValves(sRSQ[0], sRSQ[1], sRSQ[2])
        safeModule.setRangePressureValves(sRRQ[0], sRRQ[1], sRRQ[2], sRRQ[3])
    }

    function exitSave(){
        main.profileId = profileBox.currentIndex
        main.show()
        initialize.close()
        dataSource.testRead()
    }
    // onClosing:{ // make pages in main as default
    //     main.profileId = profileBox.currentIndex
        
    //     if(true){ //cfgTest[0].start
    //         let tabPage1 = page1.createObject(stackLayout); // работает
    //         container.append(tabPage1);
    //     }

    //     main.show()
    //     initialize.close()
    // }
}
