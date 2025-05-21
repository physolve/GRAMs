import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// import Grams.dataSourceSingleton 1.0

Item {
    id: root
    // color: "#2B2B2B"
    GridLayout { // move to results
        id: gridExp
        // x: 5
        y: 5
        implicitHeight: 200
        flow: GridLayout.LeftToRight
        columns: 4
        rowSpacing: 20
        columnSpacing: 10
        // Layout.preferredWidth: 350

        Label{
            // width: 80
            text: "Эксперимент:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        ComboBox{
            id: sampleChoose
            Layout.preferredWidth: 110
            // height: 40
            model: ["a","b","c"]
            font.pointSize: 10
            onActivated: {
            }
        }

        DelayButton{
            Layout.columnSpan: 2
            Layout.preferredWidth: 200
            id: calculateLastData
            text: "Последний результат"
            font.pointSize: 10
            enabled: false
        }
        Label{
            text: "Новый эксперимент"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: exportName
            Layout.preferredWidth: 110
            font.pointSize: 10
            placeholderText: "Эксперимент..."
            horizontalAlignment: TextInput.AlignHCenter
        }
        Label{
            // width: 100
            text: "Имя образца:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: textSampleName
            Layout.preferredWidth: 110
            // property int curRunCnt:  flowToVolume ? flowToVolume.runCnt : 0
            // width: 140
            text: "" // expCalc ? expCalc.expParametersStruct.nameOfSample : + curRunCnt
            font.pointSize: 10
            // anchors.verticalCenter: parent.verticalCenter
            placeholderText: "Образец ..."
            horizontalAlignment: TextInput.AlignHCenter
        }
        Label{
            // width: 90
            text: "Начало эксперимента:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: logStart
            Layout.preferredWidth: 110
            // text: toHHMMSS(logStartVal)
            text: "00:00:00"
            font.pointSize: 10
            inputMask: "99:99:99"
            inputMethodHints: Qt.ImhTime//Qt.ImhDigitsOnly
            // anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: TextInput.AlignHCenter
        }
        Label{
            // width: 100
            text: "Конец эксперимента:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: logEnd
            Layout.preferredWidth: 110
            // text: toHHMMSS(logEndVal)
            font.pointSize: 10
            text: "00:00:00"
            inputMask: "99:99:99"
            inputMethodHints: Qt.ImhTime//Qt.ImhDigitsOnly
            // anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: TextInput.AlignHCenter
        }
        Label{
            Layout.columnSpan: 2
            // width: 100
            text: "Текущий статус: "
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }

        Label{
            // width: 80
            text: "Масса, г:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: textThickness
            Layout.preferredWidth: 110
            // text: expCalc ? expCalc.expParametersStruct.thickness : 0
            font.pointSize: 10
            validator: DoubleValidator { bottom: 1e-12; top: 10000} //; decimals: 3
            selectByMouse: true
            // anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: TextInput.AlignHCenter
        }
        Label{
            // width: 100
            text: "Плотность, г/см3:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: textDiameter
            Layout.preferredWidth: 110
            // text: expCalc ? expCalc.expParametersStruct.diameter.toExponential(3) : 0
            font.pointSize: 10
            validator: DoubleValidator { bottom: 1e-12; top: 10000} //; decimals: 3
            selectByMouse: true
            // anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: TextInput.AlignHCenter
        }
        Label{
            // width: 100
            text: "Объем, м3:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: textVolume
            Layout.preferredWidth: 110
            // text: expCalc ? expCalc.expParametersStruct.volume.toExponential(3) : 0
            font.pointSize: 10
            validator: DoubleValidator { bottom: 1e-12; top: 10000} //; decimals: 2
            selectByMouse: true
            // anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: TextInput.AlignHCenter
        }

        Label{
            // width: 100
            text: "Объем тигля, м3:"
            font.pointSize: 9
            // anchors.verticalCenter: parent.verticalCenter
        }
        TextField{
            id: temperatureValue
            Layout.preferredWidth: 110
            // text: expCalc ? expCalc.expInfoStruct.expTemperature : 0
            font.pointSize: 10
            validator: DoubleValidator { bottom: 1e-12; top: 10000} //; decimals: 2
            selectByMouse: true
            // anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: TextInput.AlignHCenter
        }
    }
}