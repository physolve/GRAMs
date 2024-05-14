import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CustomPlot

Item{
    required property var testView
    width: 450
    height: 700
    // Chart Component Object
    Component{
        id: plotPressure
        Item{
            required property string plotName 
            required property var sensorsList
            required property int m_index
            property bool isDetachable: true
            //anchors.left: parent.left; 
            implicitWidth: 450
            implicitHeight: 350
            CustomPlotItem {
                id: customPlotPressure
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                width: parent.width;  height: parent.height-50
                //implicitHeight: 300
                Component.onCompleted: {
                    initCustomPlot(0) // why 0? to show place of hraph in mnemo (set name)
                    for(const sensorName of sensorsList){
                        placePointerGraph(sensorName, _myModel.getSensor(sensorName))
                    }
                }
                Component.onDestruction: testJSString(0)
                function testJSString(num) {
                    let text = "I have been destroyed_ %1"
                    console.log(text.arg(num))
                }
            }
            Connections {
                target: _myModel 
                function onDataChanged() { 
                    customPlotPressure.updatePlot()
                }
            }
                
            RoundButton {
                id: resetPosBtn
                width: 40
                height: 40
                anchors.left: parent.left; anchors.top: customPlotPressure.bottom
                text: "⟳"
                font.pixelSize: 18
                onClicked: customPlotPressure.resetPos()
            }
            RoundButton {
                id: changeUnitBtn
                width: 40
                height: 40
                anchors.left: resetPosBtn.right; anchors.top: customPlotPressure.bottom
                text: "㍴"
                font.pixelSize: 18
                font.bold: true
                font.hintingPreference: Font.PreferNoHinting
                onClicked: customPlotPressure.resetPos()
            }
            RoundButton {
                id: detachBtn
                width: 40
                height: 40
                anchors.left: changeUnitBtn.right; anchors.top: customPlotPressure.bottom
                text: "W"
                font.pixelSize: 18
                font.bold: true
                font.hintingPreference: Font.PreferNoHinting
                visible: isDetachable
                onClicked: detachPressure(m_index)
            }
            // function testFunction(b){
            //     detachBtn.visible = b   
            // }
            // ListView.onAdd: {
            //     console.log("item added to bin")
            // }
            // ListView.onRemove: {
            //     console.log("item removed from bin")
            // }
        }  
    }
    Container{
        id: baseChartBin
        anchors.fill: parent
        contentItem: ListView{
            anchors.fill: parent
            model: baseChartBin.contentModel
            snapMode: ListView.SnapOneItem
            spacing: 5
        }
        Component.onCompleted:{
            for(const [key, value] of Object.entries(testView)) {
                // key -> name of chart
                // value -> {
                //  sensors = [strings]
                //  first_y = string (purpose)
                // }
                //item required property in plotPressure
                console.log(key)
                console.log(value.sensors)

                let chart = plotPressure.createObject(baseChartBin,{plotName: key,  sensorsList: value.sensors, m_index: baseChartBin.count})
                // baseChartBin.addItem(chart) 
            }   
        }
    }

    function detachPressure(index){ // index in argument
        let winComp = {}
        winComp[baseChartBin.itemAt(index).plotName] = {"sensors": baseChartBin.itemAt(index).sensorsList} 
        const window = plotComponent.createObject(parent, {winView: winComp})
        window.show()
    }
    Component{
        id: plotComponent
        Window{
            required property var winView
            id: plotWindow
            width: 600
            height: 450
            Pane {
                anchors.fill: parent
                Material.theme: Material.Dark
                StackView{
                    id: chartStack
                    anchors.fill: parent
                    Component.onCompleted:{
                        for(const [key, value] of Object.entries(winView)) {
                            console.log(key)
                            console.log(value.sensors)
                            push(plotPressure,{plotName: key,  sensorsList: value.sensors, m_index: 0, isDetachable: false})
                        }
                    } 
                }
            }
            onClosing:{
                // remove?
            }
        }
    }
}