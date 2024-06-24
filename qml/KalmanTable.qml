import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item{
    id: root
// public    
    //signal clicked(int row, variant rowData);  //onClicked: print('onClicked', row, JSON.stringify(rowData))
    
// private
    width: 150
    height: 150
    required property variant matrixList
    required property int dimension
    GridView{
        anchors{fill: parent;}
        interactive: contentHeight > height
        flow: GridView.FlowLeftToRight
        // anchors.margins: 2
        cellWidth: 1/dimension * root.width
        cellHeight: 0.30 * root.width
        //Layout.fillWidth: true
        //Layout.fillHeight: true
        clip: true
        // interactive: false
        model: Array.from(matrixList)
        
        delegate: Item { // cell
                    width: 50;  height: 0.27 * root.width
                    
                    TextField{
                        width: 40
                        text : modelData
                        font.pointSize: 12
                        color: 'white'
                        selectByMouse: true
                        validator: DoubleValidator { bottom: 0.000; top: 99999; decimals: 3}
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: TextInput.AlignHCenter
                        onEditingFinished:{
                            // modelData = parseFloat(text)
                            matrixList[index] = parseFloat(text)
                        }
                    }
                }
    }
}