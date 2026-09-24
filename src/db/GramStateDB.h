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
    // Имя базы PostgreSQL. Демо-режим переключает на gramstate_sim до
    // первого подключения, чтобы демо-состояния не попадали в эксперимент.
    static void setDatabaseName(const QString& name);
    static QString databaseName();
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
