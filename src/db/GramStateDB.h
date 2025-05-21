#pragma once

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

class GramStateDB {

public:
    explicit GramStateDB();
    virtual ~GramStateDB();
    static bool createConnection(QVariantList& initialTimeStamp);
    bool initDatabase();
    // bool queryTimeStamp();
    bool writeTimeStamp(const QList<double> &values);
private:
    QSqlDatabase m_gramState;
    // QSqlQuery m_query;
};
