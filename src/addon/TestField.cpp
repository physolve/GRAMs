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
    guiNode a;
    a.m_prC = m_storageNodes["BC1"].getEquilibrium();
    a.m_nodeName = "BC1";
    a.m_prA = m_storageNodes["BC1"].getPressureA();
    a.m_aName = m_storageNodes["BC1"].getNameA();
    a.m_prB = m_storageNodes["BC1"].getPressureB();
    a.m_bName = m_storageNodes["BC1"].getNameB();
    m_guiNodes << a; 
    emit guiNodesChanged();
}

void TestField::runCollapse(const QString& nodeName){
    auto node1 = m_storageNodes.take("BC1");
    auto vol1 = node1.collapse(); // Virtual Volume BC1
    // all other nodes
    auto node2 = m_storageNodes.take("BD1"); // -> BC1D1
    node2.setVolumeA(vol1.get());
    m_storageNodes.insert("BC1D1", node2);
}

QList<guiNode> TestField::getGuiNodes(){
    return m_guiNodes;
}
