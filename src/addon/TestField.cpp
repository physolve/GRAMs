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
    QString nodeKey = m_guiNodes[index].m_nodeName;
    const auto& vol_pair = m_storageNodes.take(nodeKey).collapse();
    const auto& vol1 = vol_pair.first;
    folded_volumes.insert(vol_pair.second.name, vol_pair.second);
    if(m_storageNodes.count() == 0){
        NodePressure newNode;
        newNode.setVolumeA(vol1);
        newNode.setVolumeB(VirtualVolume());
        allCollapsed = newNode;
        m_guiNodes.clear();
        emit guiCollapsedChanged();
        emit guiNodesChanged();
        return;
    }
    QMap<QString, NodePressure> newStorageNodes;
    QList<guiNode> newGuiNodes;
    NodePressure newNode;
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        newNode = node;
        newNode.setVolumeA(vol1);
        const auto& newKey = nodeKey + node.getNameB();
        newStorageNodes.insert(newKey,newNode);

        guiNode ab;
        ab.m_prC = newNode.getEquilibrium();
        ab.m_nodeName = newKey;
        ab.m_prA = newNode.getPressureA();
        ab.m_aName = newNode.getNameA();
        ab.m_prB = newNode.getPressureB();
        ab.m_bName = newNode.getNameB();
        newGuiNodes << ab;
    }
    m_storageNodes.clear();
    m_storageNodes = newStorageNodes;
    emit guiCollapsedChanged();
    m_guiNodes.clear();
    m_guiNodes = newGuiNodes;
    emit guiNodesChanged();
}

QList<guiNode> TestField::getGuiNodes() const{
    return m_guiNodes;
}

QStringList TestField::getGuiCollapsed() const{
    return folded_volumes.keys();
}

void TestField::runSplit(const QString& key){;

    auto vol2 = folded_volumes.take(key);
    QMap<QString, NodePressure> newStorageNodes;
    QList<guiNode> newGuiNodes;
    NodePressure newNode;
    if(m_storageNodes.count() > 0){
        newNode = m_storageNodes.first();
        newNode.setVolumeA(newNode.split(vol2));
        newNode.setVolumeB(vol2);
    }
    else{
        newNode.setVolumeA(allCollapsed.split(vol2));
        newNode.setVolumeB(vol2);
    }
    vol2.pressure = newNode.getPressureA();
    newStorageNodes.insert(newNode.getNameA()+newNode.getNameB(),newNode);
    guiNode ab;
    ab.m_prC = newNode.getEquilibrium();
    ab.m_nodeName = newNode.getNameA()+newNode.getNameB();
    ab.m_prA = newNode.getPressureA();
    ab.m_aName = newNode.getNameA();
    ab.m_prB = newNode.getPressureB();
    ab.m_bName = newNode.getNameB();
    newGuiNodes << ab;
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        newNode = node;
        newNode.setVolumeA(newNode.split(vol2));
        const auto& newKey = newNode.getNameA()+newNode.getNameB();
        newStorageNodes.insert(newKey,newNode);

        guiNode ab;
        ab.m_prC = newNode.getEquilibrium();
        ab.m_nodeName = newNode.getNameA()+newNode.getNameB();
        ab.m_prA = newNode.getPressureA();
        ab.m_aName = newNode.getNameA();
        ab.m_prB = newNode.getPressureB();
        ab.m_bName = newNode.getNameB();
        newGuiNodes << ab;
    }
    m_storageNodes = newStorageNodes;
    emit guiCollapsedChanged();

   
    m_guiNodes.clear();
    m_guiNodes = newGuiNodes;
    emit guiNodesChanged();
}
