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
        console.log("cfgWinow closed")
    }
    color: "#292929"
    Material.theme: Material.Dark
    Material.accent: Material.Indigo
    flags: Qt.Window | Qt.FramelessWindowHint
    width: 960
    height: 720
    title: qsTr("Настройки")
    property int bw: 5
        // The mouse area is just for setting the right cursor shape
    

    JsonData {
        id: jsonData;
    }

    property var dataTest: {}
    property var cfgTest: {}

    Component.onCompleted: {
        if (datastore) {
            jsonData.parse("data.json"); // работает
            if(jsonData.result) {
                for(var i = 0; i < jsonData.length; i++) {
                    var obj = jsonData.data[i];
                    // try append obj to jsonData
                    jsonTestModel.append({
                        id: obj.id,
                        name: obj.name,
                        family: obj.family
                    })
                }
            } else {
                console.warn("Any data has not found by enable status!")
            }
            dataTest = jsonData.data
            cfgTest = jsonData.cfg
            dataModel.clear()
            var datamodel = JSON.parse(datastore)
            for (var i = 0; i < datamodel.length; ++i) dataModel.append(datamodel[i])
        }

        fillControllers()

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
            FontLoader { id: webLoveLetter; source: "qrc:/MyApplication/fonts/LoveLetter.TTF" }
            FontLoader { id: webMomоt; source: "qrc:/MyApplication/fonts/Momоt___.ttf" }
            // Text { text: "GRAMs: 350 edition"
            //     font.family: webLoveLetter.font.family
            //     font.weight: webLoveLetter.font.weight
            //     font.pixelSize: 24
            //     color: "white"
            //     horizontalAlignment: Text.AlignHCenter
            //     Layout.alignment:Qt.AlignHCenter
            // }
            Text { text: "GRAMs"
                font.family: webMomоt.font.family
                font.weight: webMomоt.font.weight
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
                    //jsonObject[0].start = jsonBtn1.checked
                    cfgTest[0].start = jsonBtn1.checked // with additional save after all
                    
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
                    cfgTest[1].start = jsonBtn2.checked
                }
                Layout.alignment: Qt.AlignHCenter
            }
            Switch {
                id: jsonBtn3
                text: qsTr("JSON test 3")
                onToggled: {
                    cfgTest[2].start = jsonBtn3.checked // with additional save after all
                }
                Layout.alignment: Qt.AlignHCenter
            }

            GridLayout {
                columns: implicitW < parent.width ? -1 : parent.width / columnImplicitWidth
                rowSpacing: 4
                columnSpacing: 4
                Layout.alignment: Qt.AlignHCenter
                property int columnImplicitWidth: children[0].implicitWidth + columnSpacing
                property int implicitW: itemModel.count * columnImplicitWidth
                ObjectModel { // fill using frofile + profileList
                        id: itemModel
                }
                Repeater { 
                    model: itemModel
                }
            }
            Button {
                id: continueBtn
                text: qsTr("Продолжить");
                onClicked: {
                    startupFunction()
                    // var jsonCfgTest = []
                    // for (var i = 0; i < jsonStartCfg.count; ++i) jsonCfgTest.push(jsonStartCfg.get(i))
                    // jsonData.cfg = JSON.stringify(jsonCfgTest) 
                    main.show()
                    cfgWindow.close()
                }
                Layout.alignment: Qt.AlignHCenter
            }
        }   
    }
    function startupFunction(objectCfgMap){
        if(cfgTest[0].start){
            let tabPage1 = page1.createObject(stackLayout); // работает
            container.append(tabPage1);
        }
        if(cfgTest[1].start){
            let tabPage2 = page2.createObject(stackLayout); // работает
            container.append(tabPage2);
        }
        if(cfgTest[2].start){
            let tabPage3 = page3.createObject(stackLayout); // работает
            container.append(tabPage3);
        }     
    }
    function saveSetUp(){
        var datamodel = []
        for (var i = 0; i < dataModel.count; ++i) datamodel.push(dataModel.get(i))
        datastore = JSON.stringify(datamodel)
        
        var jsonToSave = []
        for (var i = 0; i < jsonTestModel.count; ++i) jsonToSave.push(jsonTestModel.get(i))
        //var jsonCfgSave = []
        // for (var i = 0; i < jsonStartCfg.count; ++i) jsonCfgSave.push(jsonStartCfg.get(i))
        // jsonData.saveJson(JSON.stringify(jsonToSave), JSON.stringify(jsonCfgSave))
        var testMap = {}
        testMap["model"] = dataTest
        testMap["startCfg"] = cfgTest
        jsonData.saveJson(testMap)
    }
    ListModel {
        id: jsonTestModel
    }
    Component{
        id: rectTest1
        Rectangle {
            id: customPlus
            implicitWidth: 120
            implicitHeight: 120
            color: "transparent"
            border.color : "steelblue" 
            border.width : 8
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
    function fillControllers(){
        let rectObj1 = rectTest1.createObject();
        let rectObj2 = rectTest2.createObject();
        let rectObj3 = rectTest3.createObject();
        itemModel.append(rectObj1)
        itemModel.append(rectObj2)
        itemModel.append(rectObj3)
        
        let asr =  dataSource.profileJson
        console.log(JSON.stringify(asr, null, 4))
        for (const [key, value] of Object.entries(asr)) {
            console.log(`${key}: ${value}`);
            // use here keys from profileNames in order
            // fill the rectObj1 somehow, create insides and line settings
        }
    }
}