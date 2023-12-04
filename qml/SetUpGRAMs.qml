import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QmlJson

Window {
    id: cfgWindow
    visible: true
    onClosing: {
        winld.active = false
        console.log("cfgWindow closed")
    }
    color: "#292929"
    Material.theme: Material.Dark
    Material.accent: Material.Indigo
    flags: Qt.Window | Qt.FramelessWindowHint
    width: 960
    height: 760
    title: qsTr("Настройки")
    property int bw: 5
        // The mouse area is just for setting the right cursor shape
    

    JsonData {
        id: jsonData;
    }

    // property var dataTest: {}
    // property var cfgTest: {}

    Component.onCompleted: {
        // if (datastore) {
        //     jsonData.parse("data.json"); // работает
        //     if(jsonData.result) {
        //         for(var i = 0; i < jsonData.length; i++) {
        //             var obj = jsonData.data[i];
        //             // try append obj to jsonData
        //             jsonTestModel.append({
        //                 id: obj.id,
        //                 name: obj.name,
        //                 family: obj.family
        //             })
        //         }
        //     } else {
        //         console.warn("Any data has not found by enable status!")
        //     }
        //     dataTest = jsonData.data
        //     cfgTest = jsonData.cfg
        //     dataModel.clear()
        //     var datamodel = JSON.parse(datastore)
        //     for (var i = 0; i < datamodel.length; ++i) dataModel.append(datamodel[i])
        // }

        fillControllers(comboBoxTest.currentText)

    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: {
            const p = Qt.point(mouseX, mouseY);
            const b = bw + 10; // Increase the corner size slightly
            if (p.x < b && p.y < b) return Qt.SizeFDiagCursor;
            if (p.x >= width - b && p.y >= height - b) return Qt.SizeFDiagCursor;
            if (p.x >= width - b && p.y < b) return Qt.SizeBDiagCursor;
            if (p.x < b && p.y >= height - b) return Qt.SizeBDiagCursor;
            if (p.x < b || p.x >= width - b) return Qt.SizeHorCursor;
            if (p.y < b || p.y >= height - b) return Qt.SizeVerCursor;
        }
        acceptedButtons: Qt.NoButton // don't handle actual events
    }

    DragHandler {
        id: resizeHandler
        grabPermissions: TapHandler.TakeOverForbidden
        target: null
        onActiveChanged: if (active) {
            const p = resizeHandler.centroid.position;
            const b = bw + 10; // Increase the corner size slightly
            let e = 0;
            if (p.x < b) { e |= Qt.LeftEdge }
            if (p.x >= width - b) { e |= Qt.RightEdge }
            if (p.y < b) { e |= Qt.TopEdge }
            if (p.y >= height - b) { e |= Qt.BottomEdge }
            cfgWindow.startSystemResize(e);
        }
    }
    Page {
        anchors.fill: parent
        anchors.margins: cfgWindow.visibility === Window.Windowed ? bw : 0
        //    footer: ToolBar {
        header: ToolBar {
            //contentHeight: bw//toolButton.implicitHeight
            implicitHeight: 10
            Item {
                anchors.fill: parent
                TapHandler {
                    onTapped: if (tapCount === 2) toggleMaximized()
                    gesturePolicy: TapHandler.DragThreshold
                }
                DragHandler {
                    grabPermissions: TapHandler.CanTakeOverFromAnything
                    onActiveChanged: if (active) { cfgWindow.startSystemMove(); }
                }
            }
        }
        ColumnLayout{
            anchors.centerIn: parent
            width: parent.width
            Layout.preferredHeight:  parent.height
            visible: true
            FontLoader { id: webLoveLetter; source: "qrc:/GRAMs/fonts/LoveLetter.TTF" }
            FontLoader { id: webMomot; source: "qrc:/GRAMs/fonts/Momot___.ttf" }
            // Text { text: "GRAMs: 350 edition"
            //     font.family: webLoveLetter.font.family
            //     font.weight: webLoveLetter.font.weight
            //     font.pixelSize: 24
            //     color: "white"
            //     horizontalAlignment: Text.AlignHCenter
            //     Layout.alignment:Qt.AlignHCenter
            // }
            Text { text: "GRAMs"
                font.family: webMomot.font.family
                font.weight: webMomot.font.weight
                font.pixelSize: 24
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                //width: parent.width
                visible: true
                ComboBox {
                    id: comboBoxTest
                    width: 100
                    //implicitContentWidthPolicy: ComboBox.WidestText
                    model: dataSource.profileNames
                    onActivated: {
                        chosenTestModel.text = comboBoxTest.currentText
                        fillControllers(comboBoxTest.currentText)
                    }
                    Layout.alignment: Qt.AlignHCenter
                    Component.onCompleted: chosenTestModel.text = comboBoxTest.currentText
                }
                Label{
                    id: chosenTestModel
                    text: ""
                    font.family: webLoveLetter.font.family
                    font.weight: webLoveLetter.font.weight
                    font.pixelSize: 24
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
            }
            
            Switch {
                id: jsonBtn1
                text: qsTr("JSON test 1")
                onToggled: {
                    //jsonStartCfg.set(0, {"start": jsonBtn1.checked})
                    //console.log("Data: " + jsonData.cfg)
                    //var jsonObject = JSON.parse(jsonData.cfg)
                    //console.log("Parsed: " + jsonObject)
                    //jsonObject[0].start = jsonBtn1.checked // used to open pages in main
                    //cfgTest[0].start = jsonBtn1.checked // with additional save after all
                    
                    // var jsonObject = jsonData.cfg
                    // jsonObject[0].start = jsonBtn1.checked
                    // jsonData.cfg = jsonObject // without additional save
                    
                }
                Layout.alignment:Qt.AlignHCenter
            }
            Switch {
                id: jsonBtn2
                text: qsTr("JSON test 2")
                onToggled: {
                    //cfgTest[1].start = jsonBtn2.checked // used to open pages in main
                }
                Layout.alignment: Qt.AlignHCenter
            }
            Switch {
                id: jsonBtn3
                text: qsTr("JSON test 3")
                onToggled: {
                    //cfgTest[2].start = jsonBtn3.checked // used to open pages in main
                }
                Layout.alignment: Qt.AlignHCenter
            }

            ScrollView {
                id: scroller
                //anchors.top: parent.top
                //anchors.left: parent.left
                //anchors.leftMargin: 5
                //anchors.topMargin: 5
                width: parent.width
                //contentWidth: modelGrid.implicitW
                //height: parent.height * 0.8
                //ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
                Layout.alignment: Qt.AlignHCenter
                clip : true
                GridLayout {
                    //columns: implicitW < parent.width ? -1 : parent.width / columnImplicitWidth
                    rowSpacing: 4
                    columnSpacing: 4
                    flow: GridLayout.LeftToRight
                    columns: 3
                    rows: itemModel.count%3+1
                    //property int columnImplicitWidth: children[0].implicitWidth + columnSpacing
                    //property int implicitW: itemModel.count * columnImplicitWidth
                    ObjectModel { // fill using profile + profileList
                        id: itemModel
                    }
                    Repeater { 
                        model: itemModel
                    }
                }
            }
            Button {
                id: continueBtn
                text: qsTr("Продолжить");
                onClicked: {
                    console.log("Start")
                    // var jsonCfgTest = []
                    // for (var i = 0; i < jsonStartCfg.count; ++i) jsonCfgTest.push(jsonStartCfg.get(i))
                    // jsonData.cfg = JSON.stringify(jsonCfgTest)
                    replaceControllerInfo()
                    console.log("Pass 2")
                    fillSensors()
                    console.log("Pass 2.5")
                    // signal to c++ about creating Controllers using parameters
                    backend.initializeModel()
                    console.log("Pass 3")
                    startupFunction()
                    console.log("Pass 1")
                    main.show()
                    cfgWindow.close()
                    // change button from "Продолжить" to "Сохранить"
                }
                Layout.alignment: Qt.AlignHCenter
            }
        }   
    }
    function saveSetUp(){
        // var datamodel = []
        // for (var i = 0; i < dataModel.count; ++i) datamodel.push(dataModel.get(i))
        // datastore = JSON.stringify(datamodel)
        
        // var jsonToSave = []
        // for (var i = 0; i < jsonTestModel.count; ++i) jsonToSave.push(jsonTestModel.get(i))
        //var jsonCfgSave = []
        // for (var i = 0; i < jsonStartCfg.count; ++i) jsonCfgSave.push(jsonStartCfg.get(i))
        // jsonData.saveJson(JSON.stringify(jsonToSave), JSON.stringify(jsonCfgSave))
        // var testMap = {}
        // testMap["model"] = dataTest
        // testMap["startCfg"] = cfgTest
        // jsonData.saveJson(testMap)
        
    }
    ListModel {
        id: jsonTestModel
    }

    // Component{
    //     id: rectTest1
    //     Rectangle {
    //         id: customPlus
    //         implicitWidth: 120
    //         implicitHeight: 60
    //         color: "transparent"
    //         border.color : "steelblue" 
    //         border.width : 8
    //         property string text: "test"
    //         Text{
    //             id: customPlusText
    //             anchors.fill: parent
    //             font.pixelSize: 16
    //             text: customPlus.text
    //             horizontalAlignment: Text.AlignHCenter
    //             verticalAlignment: Text.AlignVCenter
    //         }
    //     }
    // }

    Component{
        id: valveSetUp
        ValveSetting{
        }
    }
    Component{
        id: universalSetUp
        PressureSetting{
        }
    }
    Component{
        id: profileLine
        Text {
            text: "profile string..."
            anchors.fill: parent
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    Component{
        id: rectTest2
        Rectangle { height: 30; width: 80; color: "green" }
    }
    Component{
        id: rectTest3
        Rectangle { height: 30; width: 80; color: "blue" }
    }

    property var m_sensorsMap : "" // for now, then in some qml

    function fillControllers(choosenProfile){
        itemModel.clear()
        // let rectObj1 = rectTest1.createObject();
        // let rectObj2 = rectTest2.createObject();
        // let rectObj3 = rectTest3.createObject();
        // itemModel.append(rectObj1)
        // itemModel.append(rectObj2)
        // itemModel.append(rectObj3)

        //let asr =  dataSource.profileJson
        //console.log(JSON.stringify(asr, null, 4))
        // for (const [key, value] of Object.entries(asr)) {
        //     console.log(`${key}: ${value}`);
        //     // use here keys from profileNames in order
        //     // fill the rectObj1 somehow, create insides and line settings
        // }

        let profileGRAM = dataSource.profileJson[choosenProfile]
        let controllersGRAM = profileGRAM.controllers
        // instead of Advantech, unknown and others get all controllers list
        // Value is undefined and could not be converted to an object (undefined)
        if("Advantech" in controllersGRAM){
            let advantechGRAM = controllersGRAM.Advantech
            // here u need change by profile which 
            for(const [key, value] of Object.entries(advantechGRAM)){
                console.log("I'm inside Object.entries(advantechGRAM)")

                let profileObj = universalSetUp.createObject() //???
                profileObj.color = Material.color(Material.Red)
                //Exception thrown wil::ResultException
                // think about how to connect device description with purpose
                profileObj.changePurposeFront(value.purpose)
                // change to back with 'value.device' and front with Purpose
                profileObj.setDeviceLbl(value.device) // using value[device] because it is array of obj
                if ("profile" in value){
                    console.log(key + " " + value.device + " " + value.profile) //?

                    profileObj.setDeviceProfile(value.profile) // make somewhere profiles (maybe in resources)
                }
                itemModel.append(profileObj)
                // somehow combine name, state, profile and settings
                // try fill column with objects or else flipable in separate qml
                //profileLine
                var dataArray = []
                console.log("Apply channel mapping")
                if("sensors" in value){
                    for (const element of value.sensors) {
                        dataArray.push(element.name) 
                    }
                }
                var sensorsMap = {}
                sensorsMap[value.device] = dataArray
                console.log(sensorsMap)
                m_sensorsMap = sensorsMap
            }
        }
        if("unknown" in controllersGRAM){ // option to profile without controllers
            console.log("this profile has only unknown controller")
        }

        //write compare function: 
            // every connected device compare with profile device
                //if names don't match -> red by default
                //if match -> green (continue cycle with next device)
                //if end of list -> add yellow module 
        let rsa = dataSource.connectedDevices // for now only advantech connected
        console.log("I see connected device!!!")
        console.log(Object.values(rsa))
        var map1 = []
        if("Advantech" in controllersGRAM){
            let advantechGRAM = controllersGRAM.Advantech
            console.log(advantechGRAM)
            map1 = advantechGRAM.map(a => a.device);
        }
        console.log(map1)
        for(const [key, value] of Object.entries(rsa)){
            if(map1.includes(value)){
                console.log("found "+`${value}`); 
                let index = map1.indexOf(value)
                let t_profileObj = itemModel.get(index) // take i'th item
                console.log("Now in "+ t_profileObj.innerName)
                t_profileObj.color = Material.color(Material.Green)
                let description = value+','+key
                t_profileObj.setDeviceLbl(description)
                //t_profileObj.setDeviceProfile("profile from resources") 
                t_profileObj.setDeviceConnected(true)
                console.log("set settings")
                let rsb = dataSource.deviceSettings[description]
                t_profileObj.setChannelCount(rsb.channelCount)
                t_profileObj.setValueRange(rsb.valueRanges)
                continue;
            }
            let realObj = universalSetUp.createObject() // it can be pressureSetUp
            realObj.color = Material.color(Material.Yellow)
            let description = value+','+key
            realObj.changePurposeFront("Unexpected device") // using key because it is map (already an object)
            realObj.setDeviceLbl(description) //`${value}` + " on " + `${key}` using value because it is an obj
            realObj.setDeviceProfile("profile from resources") // make somewhere profiles (maybe in resources)
            realObj.setDeviceConnected(true) // find a way to decline connection
            
            //console.log(description)
            let rsb = dataSource.deviceSettings[description]
            realObj.setChannelCount(rsb.channelCount)
            realObj.setValueRange(rsb.valueRanges)
            itemModel.append(realObj)
            // if the last key
        }
        // let t_profileObj = itemModel.get(0)
        // t_profileObj.color = Material.color(Material.Green)
        // t_profileObj.changeTextFront("I GET IT")
        // for (const [key, value] of Object.entries(rsa)) {
        //     console.log(`${key}: ${value}`);
        // }
        // for(const [key, value] of Object.entries(rsa)){
        //     let realObj = rectTest1.createObject()
        //     realObj.color = Material.color(Material.Yellow)
        //     realObj.text = key // using key because it is map (already an object)
        //     itemModel.append(realObj)
        // }
        // if(rsa.hasOwnProperty("DemoDevice,BID#0")) rectObj1.color = Material.Red;
    }
    
    function startupFunction(objectCfgMap){
        if(true){ //cfgTest[0].start
            let tabPage1 = page1.createObject(stackLayout); // работает
            container.append(tabPage1);
        }
        if(true){ //cfgTest[1].start
            let tabPage2 = page2.createObject(stackLayout); // работает
            container.append(tabPage2);
        }
        if(true){ //cfgTest[2].start
            let tabPage3 = page3.createObject(stackLayout); // работает
            container.append(tabPage3);
        }
    }
    function replaceControllerInfo(){
        console.log("inside replaceControllerInfo")
        for(var i=0; i<itemModel.count; i++){
            let t_profileObj = itemModel.get(i);
            if(t_profileObj.connected){ // now working for Pressure
                var chSet = t_profileObj.getSettings()
                console.log("innerName "+t_profileObj.innerName)
                console.log(chSet)
                dataSource.setDeviceParameters(t_profileObj.innerName, chSet)
                // let rsb = dataSource.deviceSettings[t_profileObj.innerName]
                // console.log("ChCnt "+chSet.indexChannelCount)
                // console.log("ChSt "+chSet.indexChannelStart)
                // console.log("ValRng "+chSet.indexValueRange)
            }
            else{
                console.log("I will not get inside " + t_profileObj.innerName)
            }
        }   
    }
    function fillSensors(){ // use with fillControllers
        console.log(m_sensorsMap)
        dataSource.setChannelMapping(m_sensorsMap)
    }
}