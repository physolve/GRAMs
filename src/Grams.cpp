#include "Grams.h"

#include <QDebug>
#include <QTimer>
#include <QLocale>
#include <QSysInfo>
#include <QDateTime>
#include <QTranslator>
#include <QQmlContext>
#include <QQuickWindow>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QtQuickControls2/QQuickStyle>
#include "CustomPlotItem.h"

Grams::Grams(int &argc, char **argv): 
    QApplication(argc, argv),
    initSource(),
    dataSource(),
    valveModel(),
    dataModel(),
    softTimer(new QTimer)
{

    initGUI();
    connect(softTimer, &QTimer::timeout, this, &Grams::softEvent);
}

Grams::~Grams(){

}

void Grams::initGUI(){
    QQuickStyle::setStyle("Material");
    QString applicationName = "GRAMs";
    
    // QTranslator translator;
    // const QStringList uiLanguages = QLocale::system().uiLanguages();
    // for (const QString &locale : uiLanguages) {
    //     const QString baseName = applicationName + "_" + QLocale(locale).name();
    //     if (translator.load(":/translations/" + baseName)) {
    //         app.installTranslator(&translator);
    //         break;
    //     }
    // }

    // bool openGLSupported = QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGLRhi;
    //     if (!openGLSupported) {
    //         qWarning() << "OpenGL is not set as the graphics backend, so AbstractSeries.useOpenGL will not work.";
    //         qWarning() << "Set QSG_RHI_BACKEND=opengl environment variable to force the OpenGL backend to be used.";
    //     }
    
    m_engine.addImportPath(":/");
    const QUrl url(QString("qrc:/%1/qml/main.qml").arg(applicationName));
    QObject::connect(
                &m_engine, &QQmlApplicationEngine::objectCreated, this,
                [url](QObject *obj, const QUrl &objUrl) {
                    if(!obj && url == objUrl) QCoreApplication::exit(-1);
    },
    Qt::QueuedConnection);

    // add the global styles module. Not sure why this has to be done here
    // explicitly, because we register the module in its qmldir file.
    qmlRegisterSingletonType(QUrl("qrc:/Style/Style.qml"), "Style", 1, 0, "Style");
    qmlRegisterType<CustomPlotItem>("CustomPlot", 1, 0, "CustomPlotItem");

    m_engine.rootContext()->setContextProperty("initSource", &initSource);
    m_engine.rootContext()->setContextProperty("dataSource", &dataSource);
    //m_engine.rootContext()->setContextProperty("openGLSupported", openGLSupported);
    m_engine.rootContext()->setContextProperty("_valveModel", &valveModel);
    m_engine.rootContext()->setContextProperty("_myModel", &dataModel);
    m_engine.rootContext()->setContextProperty("backend", this);
    m_engine.load(url);
}

void Grams::initializeModel(){
    //dataModel.initializeAcquisition();
    // for valves make different type
    //softTimer->start(1000);
}

void Grams::softEvent(){
    dataSource.processEvents();
    //dataModel.appendData(dataSource.getDataList());
}