#include "Grams.h"
//#include <QLocale>
//#include <QTranslator>
//#include <QDebug>

//#include <QQuickStyle>
#include <QStandardPaths>
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
        txt += QString("\tDefault: %1").arg(msg);
    }

    std::cout << "MessageHandler: " << qFormatLogMessage(type, context, txt).toStdString() << std::endl;

    //QString qs = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    //qDebug() << "write log to " << qs;

    QFile outFile(/*qs +*/ "GRAMs-log.txt");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << qFormatLogMessage(type, context, txt) << Qt::endl;
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(myMessageHandler);
    QCoreApplication::setApplicationName("GRAMs");
    QCoreApplication::setApplicationVersion("0.0.1");
    QCoreApplication::setOrganizationName(QStringLiteral("Tomsk Polytechnic University"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("tpu.ru"));
    qputenv("QT_FONT_DPI", QByteArray("128"));
    //QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    // QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    Grams app(argc, argv);
    return app.exec();
}

