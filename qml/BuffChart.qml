import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CustomPlot

Item{
    required property var testView
    // Chart Component Object
    Component{
        id: plotVoltage
        Item{
            required property string plotName 
            required property var sensorsList
            required property int m_index
            //anchors.left: parent.left; 
            implicitWidth: 1300
            implicitHeight: 600
            CustomPlotItem {
                id: customPlotVoltage
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                width: parent.width;  height: parent.height-50
                //implicitHeight: 300
                Component.onCompleted: {
                    initCustomPlot(2) // why 0? to show place of hraph in mnemo (set name)
                    placePointerGraph(sensorsList[0], filterView.getChannelSensor(0))
                }
                Component.onDestruction: testJSString(0)
                function testJSString(num) {
                    let text = "I have been destroyed_ %1"
                    console.log(text.arg(num))
                }
            }
            Connections {
                target: filterView 
                function onUpdateView() { 
                    customPlotVoltage.updatePlot()
                }
            }
                
            RoundButton {
                id: resetPosBtn
                width: 40
                height: 40
                anchors.left: parent.left; anchors.top: customPlotVoltage.bottom
                text: "⟳"
                font.pixelSize: 18
                onClicked: customPlotVoltage.resetPos()
            }
            RoundButton {
                id: changeUnitBtn
                width: 40
                height: 40
                anchors.left: resetPosBtn.right; anchors.top: customPlotVoltage.bottom
                text: "㍴"
                font.pixelSize: 18
                font.bold: true
                font.hintingPreference: Font.PreferNoHinting
                onClicked: customPlotVoltage.resetPos()
            }
        }  
    }
    StackView{
        id: chartStack
        anchors.fill: parent
        Component.onCompleted:{
            for(const [key, value] of Object.entries(testView)) {
                console.log(key)
                console.log(value.sensors)
                push(plotVoltage,{plotName: key,  sensorsList: value.sensors, m_index: 0}) // change m_index to chartStack count
            }
        } 
    }
}