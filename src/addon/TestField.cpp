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

// void TestField::setStorageNodes(const QMap<QString, NodePressure>& storageNodes){
//     m_storageNodes = storageNodes;
// }

// void TestField::setReactionNodes(const QMap<QString, NodePressure>& reactionNodes){
//     m_reactionNodes = reactionNodes;
// }

guiNode TestField::newGuiNode(const NodePressure &node, const QString& nodeKey) const{
    guiNode a;
    a.m_prC = node.getEquilibrium();
    a.m_nodeName = nodeKey;
    a.m_prA = node.getPressureA();
    a.m_aName = node.getNameA();
    a.m_prB = node.getPressureB();
    a.m_bName = node.getNameB();
    return a;
}

void TestField::updateTestField(const QMap<QString, NodePressure>& storageNodes, QStringList usedVolumes){
    previousState.clear();

    m_storageNodes = storageNodes;
    m_guiNodes.clear();
    map_guiCollapsed.clear();
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        node.update();
        m_guiNodes << newGuiNode(node, key);
    }
    emit guiCollapsedChanged();
    emit guiNodesChanged();
    auto currentStorageVolume = usedVolumes.takeFirst();
    if(usedVolumes.isEmpty()){
        return;
    }
    for(const auto& usedVolume : usedVolumes){
        currentStorageVolume+=usedVolume;
        runCollapse(currentStorageVolume); // just for storage
    }
}

void TestField::runCollapse(const QString& nodeName){
    previousState << StateCopy{m_storageNodes, m_guiNodes, map_guiCollapsed};
    
    auto node = m_storageNodes.take(nodeName);
    const auto& vol2 = node.getB();
    const auto& vol1 = node.collapse();
    map_guiCollapsed.insert(vol2.name, vol2);
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
        const auto& newKey = nodeName + node.getNameB();
        newStorageNodes.insert(newKey,newNode);
        
        newGuiNodes << newGuiNode(newNode, newKey); // external
    }
    m_storageNodes.clear();
    m_storageNodes = newStorageNodes;
    emit guiCollapsedChanged();
    m_guiNodes.clear();
    m_guiNodes = newGuiNodes;
    emit guiNodesChanged();
}

void TestField::runSplit(const QString& foldedName){
    previousState << StateCopy{m_storageNodes, m_guiNodes, map_guiCollapsed};
    auto vol2 = map_guiCollapsed.take(foldedName);
    QMap<QString, NodePressure> newStorageNodes;
    QList<guiNode> newGuiNodes;
    NodePressure newNode;
    if(m_storageNodes.count() > 0){
        newNode = m_storageNodes.first();
        newNode.setVolumeA(newNode.split(vol2));
        vol2.pressure = newNode.getPressureA();
        newNode.setVolumeB(vol2);
    }
    else{
        newNode.setVolumeA(allCollapsed.split(vol2));
        vol2.pressure = newNode.getPressureA();
        newNode.setVolumeB(vol2);
    }
    newStorageNodes.insert(newNode.getNameA()+newNode.getNameB(),newNode);
    newGuiNodes << newGuiNode(newNode, newNode.getNameA()+newNode.getNameB());
    for(const auto& [key, node] : m_storageNodes.asKeyValueRange()){
        newNode = node;
        newNode.setVolumeA(newNode.split(vol2));
        const auto& newKey = newNode.getNameA()+newNode.getNameB();
        newStorageNodes.insert(newKey,newNode);
        newGuiNodes << newGuiNode(newNode, newKey);
    }
    m_storageNodes = newStorageNodes;
    emit guiCollapsedChanged();
    m_guiNodes.clear();
    m_guiNodes = newGuiNodes;
    emit guiNodesChanged();
}

void TestField::cancelLast(){
    if(previousState.count() == 0)
        return;
    const auto& lastState = previousState.takeLast();
    m_storageNodes = lastState.buff_storageNodes;
    m_guiNodes = lastState.buff_guiNodes;
    map_guiCollapsed = lastState.buff_folded_volumes;
    emit guiCollapsedChanged();
    emit guiNodesChanged();
}

QList<guiNode> TestField::getGuiNodes() const{
    return m_guiNodes;
}

QStringList TestField::getGuiCollapsed() const{
    return map_guiCollapsed.keys();
}