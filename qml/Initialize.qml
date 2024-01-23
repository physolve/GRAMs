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
    }
    function parseRequirements(){
        let profileName = profileBox.currentText
        console.log("For " + profileName + " to work I need:")
        let curProfile = initSource.profileJson[profileName].controllers
        if("Advantech" in curProfile){
            console.log("\tI need to create Advantech instances")
            advantechRequest(curProfile.Advantech)

        }
    }
    function advantechRequest(curAdvantech){ // already sync with connected?
        console.log("Asking backend to create: ")
        let advantechParameters = []
        for(const [key, value] of Object.entries(curAdvantech)){
            switch(value.purpose){
                case "valves":
                    console.log("\tcreate valve settings for " + value.device)
                    let andvantechDO = {}
                    andvantechDO = advModuleDOType()
                    advantechParameters.push(andvantechDO)
                    break;
                case "pressure":
                case "thermocouples":
                    console.log("\tcreate thermocouples settings for " + value.device)
                    let andvantechAI = {}
                    andvantechAI = advModuleAIType()
                    advantechParameters.push(andvantechDO)
                    // creating sensors too!
                    break;
                default: console.log(`Sorry, we are out of ${value.purpose}.`);
            }
        }
    }
    function advModuleAIType(){
        initSource.advModuleAIType()

    }
    function advModuleDOType(){
        initSource.advModuleDOType()
        
    }
    onClosing:{
        main.show()
        initialize.close()
    }
}