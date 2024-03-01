#pragma once

#include <QVariant>

class Security : public QObject
{
    Q_OBJECT
    
public:
    Security(QObject *parent = 0);

    void applyProfile();
    void checkValveAction();

private:
    
};