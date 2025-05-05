#include "Grams.h"
//#include <QLocale>
//#include <QTranslator>
//#include <QDebug>

//#include <QQuickStyle>
#include <QDir>
#include <QFile>
#include <QDate>
#include <iostream>

#include <QLibraryInfo>
#include <QSettings>

void myMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg)
{
    QString txt;
    QDateTime date = QDateTime::currentDateTime(); 
    txt = date.toString("dd.MM.yyyy hh:mm:ss");
    switch (type) {
    case QtDebugMsg:
        txt += QString("\tDebug: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt += QString("\tWarning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt += QString("\tCritical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt += QString("\tFatal: %1").arg(msg);
        break;
    default:
        txt += msg;
    }

    std::cout << "MessageHandler: " << qFormatLogMessage(type, context, txt).toStdString() << std::endl;

    //QString qs = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    //qDebug() << "write log to " << qs;
    QFile outFile(/*qs +*/ "data/GRAMs-log.txt");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << qFormatLogMessage(type, context, txt) << Qt::endl;
    outFile.close();
}

int main(int argc, char *argv[]) {
    if(!QDir("data").exists()){
        QDir().mkdir("data");
    }
    qInstallMessageHandler(myMessageHandler);
    qInfo() << "Start Grams safely";
    QCoreApplication::setApplicationName("GRAMs");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName(QStringLiteral("Tomsk Polytechnic University"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("tpu.ru"));
    // pass to settings application
    // qputenv("QT_FONT_DPI", QByteArray("128")); //96/128 set for High DPI screen

    const auto &curInitProfile = QString("GRAM50");
    
    Grams app(argc, argv, curInitProfile);
    int ret;

    try{
        ret = app.exec(); 
    } catch (const std::bad_alloc &){
        // cleaning, saving session
        // close config files
        return EXIT_FAILURE;
    }
    
    return ret;
}

