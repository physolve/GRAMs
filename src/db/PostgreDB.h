#pragma once

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

static bool createConnection(QVariantList& initialTimeStamp)
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
    QVariantList lastTimeStamp;
    QSqlQuery query;
    query.exec("SELECT * FROM gramstate ORDER BY ts DESC LIMIT 1");
    while (query.next()) {
        for(auto i = 0; i < query.record().count(); ++i)
            lastTimeStamp << query.value(i);
        break; // one time?
    }
    initialTimeStamp = lastTimeStamp;
    
    /*
    INSERT INTO gramstate (ts, prSQ, prRQ, prSC1, prSC2, prSC3, prSB, prSD1, prRE, prRD2, prRF)
    VALUES (current_timestamp,
    0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0);
    */

    /*
    SELECT *
    FROM gramstate
    ORDER BY ts DESC
    LIMIT 1
    */
    //
    // Create table query is not quoted, therefore it is mapped to lower case
    // q.exec(QString("CREATE TABLE %1 (ts TIMESTAMP, id INTEGER)").arg(tableString));
    // q.exec(QString("INSERT INTO %1 (ts, id) VALUES ('%2', '3' )").arg(tableString).arg("2020-06-22 19:10:25-07"));
    // Call toLower() on the string so that it can be matched
    // QSqlRecord rec = db.record(tableString.toLower());

    db.close();
    return true;
}