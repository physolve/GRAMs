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


class TestField : public QObject
{
    Q_OBJECT // ?
    Q_PROPERTY(QList<guiNode> guiNodes READ getGuiNodes NOTIFY guiNodesChanged)
    // collapsed list
    Q_PROPERTY(QList<guiNode> guiCollapsed READ getGuiCollapsed NOTIFY guiCollapsedChanged)
public:
    TestField(QObject *parent = nullptr);
    ~TestField();
    void setStorageNodes(const QMap<QString, NodePressure>& storageNodes);
    void setReactionNodes(const QMap<QString, NodePressure>& reactionNodes);
    void setInitialNodes();
    void runTest();
    QList<guiNode> getGuiNodes();
    Q_INVOKABLE void runCollapse(const int& index); //const QString& nodeName // second, main is known
    QList<guiNode> getGuiCollapsed();
    Q_INVOKABLE void runSplit(const int& index);
signals:
    void guiNodesChanged();
    void guiCollapsedChanged();
private:
    QMap<QString, NodePressure> m_storageNodes;
    QMap<QString, NodePressure> m_reactionNodes;
    QMap<QString, NodePressure> m_initialNodes;
    QList<guiNode> m_guiNodes; // to Collapse
    // other list to split?
    QList<guiNode> m_guiCollapsed;
};