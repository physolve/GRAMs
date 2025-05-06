#pragma once
// #include <QObject>
#include "../Constants.h"
#include "NodePressure.h"
// let's make it as experiment object with file saves

struct guiNode{ // sample
    Q_GADGET
    Q_PROPERTY(QString nodeName MEMBER m_nodeName)
    Q_PROPERTY(double prC MEMBER m_prC)
    Q_PROPERTY(QString aName MEMBER m_aName)
    Q_PROPERTY(double prA MEMBER m_prA)
    Q_PROPERTY(QString bName MEMBER m_bName)
    Q_PROPERTY(double prB MEMBER m_prB)
public:
    QString m_nodeName;
    double m_prC;
    QString m_aName;
    double m_prA;
    QString m_bName;
    double m_prB;
};

struct StateCopy {
    // previous state copy :(
    QMap<QString, NodePressure> buff_storageNodes;
    QList<guiNode> buff_guiNodes;
    QMap<QString, VirtualVolume> buff_folded_volumes;
    
    // Pointer to the next node
    struct StateCopy* next;
};

class TestField : public QObject
{
    Q_OBJECT // ?
    Q_PROPERTY(QList<guiNode> guiNodes READ getGuiNodes NOTIFY guiNodesChanged)
    // collapsed list
    Q_PROPERTY(QStringList guiCollapsed READ getGuiCollapsed NOTIFY guiCollapsedChanged)
public:
    TestField(QObject *parent = nullptr);
    ~TestField();
    void setStorageNodes(const QMap<QString, NodePressure>& storageNodes);
    void setReactionNodes(const QMap<QString, NodePressure>& reactionNodes);
    void runTest();
    
    QList<guiNode> getGuiNodes() const;
    Q_INVOKABLE void runCollapse(const int& index); //const QString& nodeName // second, main is known
    QStringList getGuiCollapsed() const;
    Q_INVOKABLE void runSplit(const QString& key);
signals:
    void guiNodesChanged();
    void guiCollapsedChanged();
private:
    QMap<QString, NodePressure> m_storageNodes;
    QMap<QString, NodePressure> m_reactionNodes;

    guiNode newGuiNode(const NodePressure &node, const QString& nodeKey) const;
    QList<guiNode> m_guiNodes;

    NodePressure allCollapsed;
    QMap<QString, VirtualVolume> folded_volumes;
};