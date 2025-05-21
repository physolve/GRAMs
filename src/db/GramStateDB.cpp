#include "GramStateDB.h"
#include <QDateTime>

bool GramStateDB::createConnection(QVariantList& initialTimeStamp)
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

    // QString tableString("gramstate");
    QVariantList lastTimeStamp;
    QSqlQuery query;
    bool responseOK = query.exec("SELECT * FROM gramstate ORDER BY ts DESC LIMIT 1");
    if(!responseOK){
        qDebug() << query.lastError();
        db.close();
        return false;
    }
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

GramStateDB::GramStateDB(){

}

GramStateDB::~GramStateDB(){
    if(m_gramState.isOpen()){
        m_gramState.close();
    }
}

bool GramStateDB::initDatabase(){
    m_gramState = QSqlDatabase::database();
    m_gramState.setHostName("localhost"); // ?
    m_gramState.setDatabaseName("gramstate");
    m_gramState.setUserName("gramapp");
    m_gramState.setPassword("fast");
    if (!m_gramState.open()) {
        qDebug() << "Cannot open database";
        return false;
    }
    // m_query = QSqlQuery(m_gramState);
    return true;
}

// bool GramStateDB::queryTimeStamp(){
//     if(!m_gramState.isOpen()){
//         return false;
//     }
//     return true;
//     return m_query.prepare("INSERT INTO gramstate (ts, prSQ, prRQ, prSC1, prSC2, prSC3, prSB, prSD1, prRE, prRD2, prRF) "
//                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
//                     //"VALUES (:ts, :prSQ, :prRQ, :prSC1, :prSC2, :prSC3, :prSB, :prSD1, :prRE, :prRD2, :prRF)");
// }

bool GramStateDB::writeTimeStamp(const QVector<double> &values){
    if(!m_gramState.isOpen()){
        return false;
    }
    QSqlQuery query;
    query.prepare("INSERT INTO gramstate (ts, prSQ, prRQ, prSC1, prSC2, prSC3, prSB, prSD1, prRE, prRD2, prRF) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    // qDebug() << QDateTime::currentDateTime();
    query.addBindValue(QDateTime::currentDateTime().toString()); // utc?
    for(const double& value: values){
        query.addBindValue(value);
    }
    return query.exec();
}