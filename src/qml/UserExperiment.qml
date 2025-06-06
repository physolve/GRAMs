import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "lumber"

Rectangle {
    id: userExperiment
    color: "#464646"
    border.color: "#F2C029"
    Row{
        x: 25
        y: 25
        spacing: 15
        Text{
            // width: parent.width
            text: "Пользователь"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            font.family: "Verdana"
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
        }
        Button{
            height: 40
            width: 120
            text: "id0 ХалеевДЕ"
            checked: true
        }
    }
    Row{
        x: 300
        y: 25
        spacing: 15
        Text{
            // width: parent.width
            text: "Камера"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            font.family: "Verdana"
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
        }
        Button{
            height: 40
            width: 120
            text: "US420"
            checked: true
        }
    }
    Row{
        x: 25
        y: 90
        spacing: 15
        Text{
            // width: parent.width
            text: "Образец"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            font.family: "Verdana"
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
        }
        Button{
            height: 40
            width: 120
            text: "LaNi₅"
            checked: true
        }
    }
    Row{
        x: 300
        y: 90
        spacing: 15
        Text{
            // width: parent.width
            text: "Тигель"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            font.family: "Verdana"
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
        }
        Button{
            height: 40
            width: 120
            text: "№1"
            checked: true
        }
    }
    Row{
        x: 25
        y: 150
        spacing: 15
        Text{
            // width: parent.width
            text: "Имя эксперимента"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            font.family: "Verdana"
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
        }
        TextField{
            // width: parent.width - 80
            width: 190
            height: 40
            placeholderText: "05-06-2025_"
            font { family: 'Courier'; pointSize: 10; }
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
        }
        Button{
            width: 90
            text: "Загр"
        }
    }
    Rectangle{
        x: 25
        y: 223
        width: parent.width - 50
        height: 454
        color: "transparent"
        border.color: "#ABDBDD"
        border.width: 2
        radius: 5
    }
    HorizontalHeaderView {
        id: horizontalHeader
        syncView: playTable
        x: 25
        y: 225
        // implicitHeight: 36
        model: ["Режим", "Условие", "Повтор", "Макс. время"]
        clip: true
        // delegate: Rectangle {
        //     color: "white"
        //     Text { text: modelData; color: "black" }
        // }
        // movableColumns: false
    }
    ScrollView {
        id: tableScrollExp
        x: 25
        y: 275
        height: 400
        width: parent.width-50
        // property bool activeScrollBar: ScrollBar.vertical.active
        PlayTable{
            id: playTable
            // topMargin: horizontalHeader.implicitHeight+5
            leftMargin: 5
            columnWidths: [80, 230, 55, 80, 0]
            // onClicked: function(row, rowData) { print('onClicked', row, JSON.stringify(rowData)); }
        }
        SelectionRectangle {
            target: playTable
        }
        // contentWidth: children.implicitWidth
        // contentHeight: children.implicitHeight
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        function scrollToBottom(){
            ScrollBar.vertical.position = 1.0 - ScrollBar.vertical.size
        }
        clip: true
        // onActiveScrollBarChanged:{
        //     background.color = "black"
        // }
    }
    // ComboBox{
    //     id: addRegime
    //     x: 50
    //     y: 630
    //     width: 200
    //     height: 50
    //     visible: false
    //     model: ["Вакуум", "SOAK...", "PCI...", "Проницаемость", "ТДС", "Наблюдение", "Калибровка...", "Проверка системы", "Цел. в камере"]
    // }
    function addRegime(regime){
        playTable.model.appendRow({
                name: regime,
                passed: 0,
                from: 3,
                time: "00:00:00",
                setting: 0.0
            })
        tableScrollExp.scrollToBottom()
    }
    MenuBar {
        id: addRegime
        x: 50
        y: 630

        Menu{
            title: "Добавить"
            MenuItem {
                text: "Вакуум"
                onTriggered: userExperiment.addRegime(text)
            }
            Menu {
                cascade: true  // Nested menu
                title: "SOAK"
                MenuItem { text: "Спадающий"; onTriggered: userExperiment.addRegime(`SOAK ${text}` ) }
                MenuItem { text: "Постоянный"; onTriggered: userExperiment.addRegime(text) }
                MenuItem { text: "Десорбция"; onTriggered: userExperiment.addRegime(text) }
            }
            Menu {
                cascade: true  // Nested menu
                title: "PCI"
                MenuItem { text: "Сорбция"; onTriggered: userExperiment.addRegime(text) }
                MenuItem { text: "Десорбция"; onTriggered: userExperiment.addRegime(text) }
            }
            MenuItem {
                text: "Проницаемость"
            }
            MenuItem {
                text: "ТДС"
            }
            MenuItem {
                text: "Наблюдение"
                onTriggered: userExperiment.addRegime(text)
            }
            MenuItem {
                text: "Калибровка"
            }
            MenuItem {
                text: "Проверка системы"
            }
            MenuItem {
                text: "Цел. в камере"
            }
        }
    }
    MenuBar {
        id: deleteRegime
        x: 150
        y: 630

        Menu{
            title: "Удалить"
        }
    }
    // Button{
    //     x: 50
    //     y: 675
    //     width: parent.width-100
    //     height: 50
    //     text: "Добавить"
    //     onClicked:{
    //         playTable.model.appendRow({
    //             runCnt: 0,
    //             name: 1,
    //             passed: 2,
    //             from: 3,
    //             chargeFrom: 4,
    //             to: 5
    //         })
    //         tableScrollExp.scrollToBottom()
    //         addRegime.popup.open()
    //     }
    // }
    
    Row{
        // x: 25
        y: 800
        spacing: 15
        anchors.horizontalCenter: parent.horizontalCenter
        Button{
            height: 40
            width: 120
            text: "Применить"
        }
        Button{
            height: 40
            width: 120
            text: "Сохранить"
            enabled: false
        }
        Button{
            height: 40
            width: 120
            text: "Очистить"
        }
    }
}