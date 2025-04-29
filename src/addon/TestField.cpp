#include "TestField.h"

#include <QDebug>

TestField::TestField(QObject *parent) :
    QObject(parent)
{
    qDebug() << "TestField class is created";
}

TestField::~TestField()
{
    qDebug() << "TestField class is destroyed";
}

void TestField::setStorageNodes(const QMap<QString, NodePressure>& storageNodes){
    m_storageNodes = storageNodes;
}

void TestField::setReactionNodes(const QMap<QString, NodePressure>& reactionNodes){
    m_reactionNodes = reactionNodes;
}

void TestField::runTest(){
    // lets join B and C1:
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        guiNode a;
        node.update();
        a.m_prC = node.getEquilibrium();
        a.m_nodeName = key;
        a.m_prA = node.getPressureA();
        a.m_aName = node.getNameA();
        a.m_prB = node.getPressureB();
        a.m_bName = node.getNameB();
        m_guiNodes << a;
    }

    emit guiNodesChanged();
}

void TestField::runCollapse(const int& index){
    qDebug() << index;
    const QString& nodeKey = m_guiNodes[index].m_nodeName;

    if(m_storageNodes.count() == 1){
        qDebug() << "No more Nodes";
        return;
    }
    auto node1 = m_storageNodes.take(nodeKey);
    // put it to collapsed list!
    auto vol1 = node1.collapse(); // node1.collapse(); // Virtual Volume BC1
    // all other nodes, if they are
    QMap<QString, NodePressure> newStorageNodes;
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        NodePressure newNode = node;
        newNode.setVolumeA(vol1);
        const auto& newKey = nodeKey + node.getNameB();
        newStorageNodes.insert(newKey,newNode);     
    }
    m_storageNodes.clear();
    m_storageNodes = newStorageNodes;
    // const auto& [name, list] : contradictionValves.asKeyValueRange()){
    // auto node2 = m_storageNodes.take("BD1"); // -> BC1D1
    // node2.setVolumeA(vol1.get());
    // m_storageNodes.insert("BC1D1", node2);
    
    m_guiNodes.remove(index);
    QList<guiNode> newGuiNodes;
    for(const auto& nodeGui : m_guiNodes){
        // if(nodeGui.m_nodeName == "BC1"||nodeGui.m_nodeName == "BD1"){
        //     m_guiNodes.removeOne(nodeGui);
        // }
        const auto& newKey = nodeKey + nodeGui.m_bName;
        m_storageNodes[newKey].update();
        const auto& newNode = m_storageNodes[newKey];
        guiNode ab;
        ab.m_prC = newNode.getEquilibrium();
        ab.m_nodeName = newKey;
        ab.m_prA = newNode.getPressureA();
        ab.m_aName = newNode.getNameA();
        ab.m_prB = newNode.getPressureB();
        ab.m_bName = newNode.getNameB();
        newGuiNodes << ab;
    }
    m_guiNodes.clear();
    m_guiNodes = newGuiNodes;
    emit guiNodesChanged();
}

QList<guiNode> TestField::getGuiNodes(){
    return m_guiNodes;
}
