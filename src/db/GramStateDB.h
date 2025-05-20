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
    bool setTimeStamp();
private:
    QSqlDatabase gramState;

};
