#pragma once

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "../DataCollection.h"

class GramStateDB {

public:
    explicit GramStateDB();
    virtual ~GramStateDB();
    static bool createConnection(QVariantList& initialTimeStamp);
    bool initDatabase();
    // bool queryTimeStamp();
    // bool writeTimeStamp(const QList<double> &values);
    void setTimeStampDataPointers(const QVector<DataCollection*>& ptr);
    bool writeTimeStamp();
private:
    QSqlDatabase m_gramState;
    // QSqlQuery m_query;
    QVector<DataCollection*> m_timeStampDataPointers;
};
