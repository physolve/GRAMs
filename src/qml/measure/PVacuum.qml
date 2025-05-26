import QtQuick
import QtQuick.Controls
import Grams.dataSourceSingleton 1.0

Item {
    id: root
    // color: "#2B2B2B"
    Row{
        x: 25
        y: 80
        spacing: 10
        
        Text{
            // width: parent.width
            text: "Откачать до"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        ComboBox{
            id: sampleChoose
            // height: 40
            model: ["Бочка","Эталонного","Реакционного","Камеры"]
            font.pointSize: 10
            onActivated: {
            }
        }
    }
    Row{
        x: 25
        y: 170
        spacing: 10

        Text{
            // width: parent.width
            text: "Целевое"
            font.family: "Verdana"
            horizontalAlignment: Text.AlignHCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
        TextField{
            // width: parent.width - 80
            width: 75
            height: 35
            readOnly: true
            text: "0"
            placeholderText: "бар"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
    }
    Button{
        x: 25
        y: 210
        text: "Test query"
        onClicked: DataSource.testVacuumQuery()
    }
}