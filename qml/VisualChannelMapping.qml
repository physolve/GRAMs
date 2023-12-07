import QtQuick
import QtQuick.Controls
import QtQml.Models

Rectangle {
    id: visualChannelMapping
    //first row is channels
    //second row is sensors with arrows to left and right transposition
    
    width: 320
    height: 320
    
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#f6f6f6" }
        GradientStop { position: 1.0; color: "#d7d7d7" }
    }
    
    ObjectModel {
        id: itemModel
        
        Rectangle { height: 60; width: 80; color: "#157efb" }
        Rectangle { height: 20; width: 300; color: "#53d769" 
            Text { anchors.centerIn: parent; color: "black"; text: "Hello QML" }
        }
        Rectangle { height: 40; width: 40; radius: 10; color: "#fc1a1c" }
    }
    
    ListView {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
        
        model: itemModel
    }
    
    /*

    */
    // GridLayout {
    //     //columns: implicitW < parent.width ? -1 : parent.width / columnImplicitWidth
    //     rowSpacing: 4
    //     columnSpacing: 4
    //     flow: GridLayout.LeftToRight
    //     columns: 3
    //     rows: 1//itemModel.count%3+1
    //     //property int columnImplicitWidth: children[0].implicitWidth + columnSpacing
    //     //property int implicitW: itemModel.count * columnImplicitWidth
    //     ObjectModel { // fill using profile + profileList
    //         id: namesModel
    //     }
    //     Repeater { 
    //         model: namesModel
    //     }
    // }
}