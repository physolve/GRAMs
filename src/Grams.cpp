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
#include <QQuickStyle>
#include "CustomPlotItem.h"

Grams::Grams(int &argc, char **argv, const QString &curInitProfile): 
    QApplication(argc, argv),
    initSource(),
    dataSource(),
    valveModel(), // replace
    dataModel(), // replace
    m_safeModule() // check
{
    initDigitalData();
    advDoController();
    initGUI();
}

Grams::~Grams(){
    qInfo() << "Exit Grams safely \n\n";
}

void Grams::initDigitalData(){
    vAR1.m_name = "Supply port 1"; // later from profile
    vAR2.m_name = "Supply port 2";
    vAR3.m_name = "Supply port 3";
    vAR4.m_name = "Gas drain atm";
    vAR5.m_name = "Gas drain barrel";
    vAR6.m_name = "Gas drain vacuum";

    vS1.m_name = "Little storage";
    vS2.m_name = "Normal storage";
    vS3.m_name = "Large storage";
    vS4.m_name = "Pressure range storage";

    vR1.m_name = "Leakage slow";
    vR2.m_name = "Leakage fast";
    vR3.m_name = "Leakage tube";
    vR4.m_name = "Pressure range reaction";
    vR5.m_name = "Chamber manual valve";

    vSL1.m_name = "Second line outlet";
    vSL2.m_name = "Second line barrel";

}

void Grams::advDoController(){
    if(!initSource.isInitializeOk())
        return;
    // pointers to valves
    Valve *valveList[16] = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    dataSource.setValvePointers(*valveList, 16);
    QList<daqParameters> parameters;
    initSource.profileToRealParameters(parameters);    
    // dataSource. set Required
    dataSource.initDaq(parameters);
    emit valveChanged();
}

void Grams::setValveState(bool state, int index){
    // signal from GUI to change state of object
    Valve *valveList[16] = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    
    if(true) {// securityCheck ok?
        valveList[index]->m_state = state;
        dataSource.setValveStates();
    }
    emit valveChanged();
}

void Grams::initGUI(){
    QQuickStyle::setStyle("Material");
    QString applicationName = "GRAMs"; // curInitProfile also?
    
    m_engine.addImportPath(":/");
    const QUrl url(QString("qrc:/%1/qml/Main.qml").arg(applicationName));
    QObject::connect(
                &m_engine, &QQmlApplicationEngine::objectCreated, this,
                [url](QObject *obj, const QUrl &objUrl) {
                    if(!obj && url == objUrl) QCoreApplication::exit(-1);
    },
    Qt::QueuedConnection);
    
    qmlRegisterType<CustomPlotItem>("CustomPlot", 1, 0, "CustomPlotItem");

    m_engine.rootContext()->setContextProperty("initSource", &initSource);
    m_engine.rootContext()->setContextProperty("dataSource", &dataSource);
    //m_engine.rootContext()->setContextProperty("openGLSupported", openGLSupported);
    m_engine.rootContext()->setContextProperty("_valveModel", &valveModel);
    m_engine.rootContext()->setContextProperty("_myModel", &dataModel);
    m_engine.rootContext()->setContextProperty("safeModule", &m_safeModule);

    // create somewhere filterview
    // m_engine.rootContext()->setContextProperty("filterView", dataSource.getFilterView()); // initialize here and pass to dataSource

    m_engine.rootContext()->setContextProperty("backend", this);
    m_engine.load(url);
}

// void Grams::initializeReading(){
//     // for valves make different type

//     // Add variable timer msec
//     dataModel.initializeAcquisition();
//     readingEvent(true);
//     softTimer->start(1000);
//     qDebug() << "Now I'm updating every 1000 ms";
// }

void Grams::softEvent(){

    //for now only reading event
    readingEvent(false);

}

void Grams::readingEvent(bool valveCheck){
    if(dataSource.getGRAMsIntegrity())
        dataSource.processEvents();
    
    dataSource.processEvents("pressure");

    // dataModel.appendData(dataSource.getMeasures()); // don't like it
    
    // if(valveCheck)
    //     valveModel.appendData(dataSource.getValves()); // always valve check? why
}

// void Grams::setValveState(const QString &name, const bool &state){ // should be filtered
//     // find a way for force valve set (as SU)
//     if(!dataSource.getGRAMsIntegrity())
//         return;
//     qDebug() << name << " " << state;
//     auto valveMap = valveModel.getValveMap();
//     auto result = m_safeModule.checkValveAction(valveMap, name, state);
//     valveModel.appendData(name,result);
//     auto valveVector = valveModel.getValveVector();
//     qDebug() << valveVector;
//     dataSource.setValves(valveVector);
//     readingEvent(true);
// }

bool Grams::getVAR1State() const{
    return vAR1.m_state;
}
bool Grams::getVAR2State() const{
    return vAR2.m_state;
}
bool Grams::getVAR3State() const{
    return vAR3.m_state;
}
bool Grams::getVAR4State() const{
    return vAR4.m_state;
}
bool Grams::getVAR5State() const{
    return vAR5.m_state;
}
bool Grams::getVAR6State() const{
    return vAR6.m_state;
}
bool Grams::getVS1State() const{
    return vS1.m_state;
}
bool Grams::getVS2State() const{
    return vS2.m_state;
}
bool Grams::getVS3State() const{
    return vS3.m_state;
}
bool Grams::getVS4State() const{
    return vS4.m_state;
}
bool Grams::getVR1State() const{
    return vR1.m_state;
}
bool Grams::getVR2State() const{
    return vR2.m_state;
}
bool Grams::getVR3State() const{
    return vR3.m_state;
}
bool Grams::getVR4State() const{
    return vR4.m_state;
}
bool Grams::getVR5State() const{
    return vR5.m_state;
}
bool Grams::getVSL1State() const{
    return vSL1.m_state;
}
bool Grams::getVSL2State() const{
    return vSL2.m_state;
}
