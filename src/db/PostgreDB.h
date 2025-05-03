#pragma once

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

static bool createConnection()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost"); // ?
    db.setDatabaseName("gramstate");
    db.setUserName("gramapp");
    db.setPassword("fast");

    if (!db.open()) {
        qDebug() << "Cannot open database";
        return false;
    }

    QString tableString("gramstate");
    
    QSqlQuery q;
    // Create table query is not quoted, therefore it is mapped to lower case
    // q.exec(QString("CREATE TABLE %1 (ts TIMESTAMP, id INTEGER)").arg(tableString));
    // q.exec(QString("INSERT INTO %1 (ts, id) VALUES ('%2', '3' )").arg(tableString).arg("2020-06-22 19:10:25-07"));
    // Call toLower() on the string so that it can be matched
    // QSqlRecord rec = db.record(tableString.toLower());
    return true;
}