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

void TestField::setInitialNodes(){
    m_initialNodes.insert(m_storageNodes);
    m_initialNodes.insert(m_reactionNodes);
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
    QString nodeKey = m_guiNodes[index].m_nodeName;

    if(m_storageNodes.count() == 1){
        qDebug() << "No more Nodes";
        return;
    }
    auto node1 = m_storageNodes.take(nodeKey);
    
    auto vol1 = node1.collapse();
    // m_guiCollapsed << m_guiNodes[index];

    QMap<QString, NodePressure> newStorageNodes;
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        NodePressure newNode = node;
        newNode.setVolumeA(vol1);
        const auto& newKey = nodeKey + node.getNameB();
        newStorageNodes.insert(newKey,newNode);     
    }
    m_storageNodes.clear();
    m_storageNodes = newStorageNodes;
    // m_guiNodes.remove(index);
    m_guiCollapsed << m_guiNodes.takeAt(index);
    emit guiCollapsedChanged();

    QList<guiNode> newGuiNodes;
    for(const auto& nodeGui : m_guiNodes){
        const auto& newKey = nodeKey + nodeGui.m_bName;
        // m_storageNodes[newKey].update();
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

QList<guiNode> TestField::getGuiCollapsed(){
    return m_guiCollapsed;
}

void TestField::runSplit(const int& index){
    qDebug() << index;
    QString nodeSplitKey = m_guiCollapsed[index].m_bName;
    QMap<QString, NodePressure> newStorageNodes;
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        const auto& nodeToSplit = m_initialNodes[nodeSplitKey];
        node.split(nodeToSplit.);
        
        NodePressure newNode = node;
        newNode.setVolumeA(vol1);
        const auto& newKey = nodeKey + node.getNameB();
        newStorageNodes.insert(newKey,newNode);     
    }



    auto node1 = m_storageNodes.take(nodeKey);
}
