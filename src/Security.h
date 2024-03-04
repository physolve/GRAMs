#pragma once

#include <QVariant>

struct ValveGraph{
    //security options and twoOfThreeRule
};

class Security : public QObject
{
    Q_OBJECT
    Q_INVOKABLE void setContradictionValves(const QVariantMap &contradictionValves);
public:
    Security(QObject *parent = 0);

    void applyProfile();
    void checkValveAction();

private:
    // current states (valves)
    // current pressure
    // incoming states (valves)
    // incoming pressure
    // filter incoming states to current states
    // filter incoming states to current/incoming pressure
    QMap<QString, ValveGraph> m_contradictionValves;
};