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

Grams::Grams(int &argc, char **argv, const QString &curInitProfile): 
    QApplication(argc, argv),
    initSource(),
    dataSource(),
    // valveModel(), // replace
    // dataModel(), // replace
    m_safeModule() // check
{
    initDigitalData();
    initAnalogData();
    advDoController();
    advAiController();
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

void Grams::initAnalogData(){
    // prSH = ControllerData() parameters from easy get hardwareParameters Sensor
    ControllerData* pressureSensorsList[8] = {&prSH, &prSA, &prRH, &prRA, &prRL, &prSK, &tmSK, &tmS};
    ControllerData* tempSensorsList[8] = {&tmX, &tmY, &tmSLittle, &tmSSmall, &tmSLarge, &tmSTube, &tmRTube, &tmF};
    const auto& pressureSensors = initSource.getPressureSensors();
    for(int i = 0; i < 8; ++i){
        const auto& pressureSensor = pressureSensors[i];
        pressureSensorsList[i]->m_name = pressureSensor.m_sensorName;
        pressureSensorsList[i]->setCoeffs(pressureSensor.m_A*1000.0/pressureSensor.m_R, pressureSensor.m_B);
    }
    const auto& tempSensors = initSource.getTempSensors();
    for(int i = 0; i < 8; ++i){
        tempSensorsList[i]->m_name = tempSensors[i];
        tempSensorsList[i]->setCoeffs(1.0, 0.0);
    }
}

void Grams::advDoController(){
    if(!initSource.isInitializeOk())
        return;
    // pointers to valves
    Valve *valveList[16] = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    dataSource.setValvePointers(*valveList, 16);
    daqParameters parametersDO;
    initSource.getParametersDO(parametersDO);    
    // dataSource. set Required
    dataSource.initDaqDO(parametersDO);
    emit valveChanged();
}

void Grams::advAiController(){
    if(!initSource.isInitializeOk())
        return;
    ControllerData* pressureSensorsList[8] = {&prSH, &prSA, &prRH, &prRA, &prRL, &prSK, &tmSK, &tmS};
    ControllerData* tempSensorsList[8] = {&tmX, &tmY, &tmSLittle, &tmSSmall, &tmSLarge, &tmSTube, &tmRTube, &tmF};

    daqParameters parametersAIpres;
    daqParameters parametersAItemp;

    initSource.getParametersAIpres(parametersAIpres);
    dataSource.setPressurePointers(*pressureSensorsList, 8);
    dataSource.initDaqAIpres(parametersAIpres);
    guiValsUpdate();

    initSource.getParametersAItemp(parametersAItemp);
    dataSource.setTempPointers(*tempSensorsList, 8);
    dataSource.initDaqAItemp(parametersAItemp);
}

void Grams::setValveState(bool state, int index){
    // signal from GUI to change state of object
    Valve *valveList[16] = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    const bool originalState = valveList[index]->getState();
    if(true) {// securityCheck ok? || state != originalState
        valveList[index]->setState(state);
        if(!dataSource.setValveStates())
            valveList[index]->setState(originalState);
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

    qmlRegisterSingletonInstance("Grams.dataSourceSingleton", 1, 0, "DataSource", &dataSource);
    
    m_engine.rootContext()->setContextProperty("initSource", &initSource); // make singleton later
    
    // m_engine.rootContext()->setContextProperty("dataSource", &dataSource);
    //m_engine.rootContext()->setContextProperty("openGLSupported", openGLSupported);
    // m_engine.rootContext()->setContextProperty("_valveModel", &valveModel);
    // m_engine.rootContext()->setContextProperty("_myModel", &dataModel);

    m_engine.rootContext()->setContextProperty("safeModule", &m_safeModule);

    // create somewhere filterview
    // m_engine.rootContext()->setContextProperty("filterView", dataSource.getFilterView()); // initialize here and pass to dataSource

    m_engine.rootContext()->setContextProperty("backend", this);
    m_engine.load(url);
}

void Grams::guiValsUpdate(){
    m_pressureVals.g_prSH = prSH.getCurValue();
    m_pressureVals.g_prSA = prSA.getCurValue();
    m_pressureVals.g_prRH = prRH.getCurValue();
    m_pressureVals.g_prRA = prRA.getCurValue();
    m_pressureVals.g_prRL = prRL.getCurValue();
    m_pressureVals.g_prSK = prSK.getCurValue();
    m_pressureVals.g_tmSK = tmSK.getCurValue();
    m_pressureVals.g_tmS = tmS.getCurValue();
    emit guiValsPresChanged();
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

guiValues Grams::getGuiValsPres() const{
    return m_pressureVals;
}

bool Grams::getVAR1State() const{
    return vAR1.getState();
}
bool Grams::getVAR2State() const{
    return vAR2.getState();
}
bool Grams::getVAR3State() const{
    return vAR3.getState();
}
bool Grams::getVAR4State() const{
    return vAR4.getState();
}
bool Grams::getVAR5State() const{
    return vAR5.getState();
}
bool Grams::getVAR6State() const{
    return vAR6.getState();
}
bool Grams::getVS1State() const{
    return vS1.getState();
}
bool Grams::getVS2State() const{
    return vS2.getState();
}
bool Grams::getVS3State() const{
    return vS3.getState();
}
bool Grams::getVS4State() const{
    return vS4.getState();
}
bool Grams::getVR1State() const{
    return vR1.getState();
}
bool Grams::getVR2State() const{
    return vR2.getState();
}
bool Grams::getVR3State() const{
    return vR3.getState();
}
bool Grams::getVR4State() const{
    return vR4.getState();
}
bool Grams::getVR5State() const{
    return vR5.getState();
}
bool Grams::getVSL1State() const{
    return vSL1.getState();
}
bool Grams::getVSL2State() const{
    return vSL2.getState();
}