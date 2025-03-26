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
    m_safeModule(), // check
    softTimer(new QTimer)
{
    initDigitalData();
    initAnalogData();
    advDoController();
    advAiController();

    initAddRemoveQuartile();
    initStorageQuartile();

    initGUI();
    initSafeModule();
    connect(softTimer, &QTimer::timeout, this, &Grams::softEvent);
    softTimer->setInterval(500);
    softTimer->start();
    dataSource.startAcquisition();
}

Grams::~Grams(){
    qInfo() << "Exit Grams safely \n\n";
}

void Grams::initDigitalData(){
    vAR1.m_name = "AR1"; // "Supply port 1"; // later from profile
    vAR2.m_name = "AR2"; // "Supply port 2";
    vAR3.m_name = "AR3"; // "Supply port 3";
    vAR4.m_name = "AR4"; // "Gas drain atm";
    vAR5.m_name = "AR5"; // "Gas drain barrel";
    vAR6.m_name = "AR6"; // "Gas drain vacuum";

    vS1.m_name = "S1"; // "Little storage";
    vS2.m_name = "S2"; // "Normal storage";
    vS3.m_name = "S3"; // "Large storage";
    vS4.m_name = "S4"; // "Pressure range storage";

    vR1.m_name = "R1"; // "Leakage slow";
    vR2.m_name = "R2"; // "Leakage fast";
    vR3.m_name = "R3"; // "Leakage tube";
    vR4.m_name = "R4"; // "Pressure range reaction";
    vR5.m_name = "R5"; // "Chamber manual valve";

    vSL1.m_name = "SL1"; // "Second line outlet";
    vSL2.m_name = "SL2"; // "Second line barrel";

    timeAnalog.m_name = "Time";

    QVector<double> indexData;
    for(int i = 0; i < 512; ++i) { //m_sectionLength
        indexData << i;
    }
    timeFilter.setData(indexData);
    for(auto filtersData : getFilterPointers()){
        filtersData->setData(QVector<double>(512,0.0));
    }
}

void Grams::initAnalogData(){
    // prSH = ControllerData() parameters from easy get hardwareParameters Sensor
    ControllerData* pressureSensorsList[8] = {&prSH, &prSA, &prRH, &prRA, &prRL, &prSK, &tmSK, &tmS};
    ControllerData* tempSensorsList[8] = {&tmX, &tmY, &tmSLittle, &tmSSmall, &tmSLarge, &tmSTube, &tmRTube, &tmF};
    const auto& pressureSensors = initSource.getPressureSensors();
    for(int i = 0; i < 8; ++i){
        const auto& pressureSensor = pressureSensors[i];
        pressureSensorsList[i]->m_name = pressureSensor.m_sensorName;
        pressureSensorsList[i]->setCoeffs(pressureSensor.m_A/pressureSensor.m_R*1000.0, pressureSensor.m_B); //*1000.0 fix profile later
    }
    const auto& tempSensors = initSource.getTempSensors();
    for(int i = 0; i < 8; ++i){
        tempSensorsList[i]->m_name = tempSensors[i];
        tempSensorsList[i]->setCoeffs(1.0, 0.0);
    }
    // mole Volume names from addons js
    m_pressureVals = guiValsPres{0,0,0,0,0,0,0,0};
    m_tempVals = guiValsTemp{0,0,0,0,0,0,0,0};
}

void Grams::advDoController(){
    if(!initSource.isInitializeOk())
        return;
    // pointers to valves
    QVector<Valve*> valveList = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    dataSource.setValvePointers(valveList);
    daqParameters parametersDO;
    initSource.getParametersDO(parametersDO);    
    // dataSource. set Required
    dataSource.initDaqDO(parametersDO);
    emit valveChanged();
}

void Grams::advAiController(){
    if(!initSource.isInitializeOk())
        return;
    QVector<ControllerData*> pressureSensorsList = {&prSH, &prSA, &prRH, &prRA, &prRL, &prSK, &tmSK, &tmS};
    QVector<ControllerData*> tempSensorsList = {&tmX, &tmY, &tmSLittle, &tmSSmall, &tmSLarge, &tmSTube, &tmRTube, &tmF};

    daqParameters parametersAIpres;
    daqParameters parametersAItemp;

    dataSource.setTimePointer(&timeAnalog);

    initSource.getParametersAIpres(parametersAIpres);
    dataSource.setPressurePointers(pressureSensorsList);
    dataSource.setFiltersDataPointers(getFilterPointers());
    dataSource.initDaqAIpres(parametersAIpres);

    initSource.getParametersAItemp(parametersAItemp);
    dataSource.setTempPointers(tempSensorsList);
    dataSource.initDaqAItemp(parametersAItemp);

    guiValsUpdate();
}

void Grams::initAddRemoveQuartile(){
    m_supplyPressureHigh.m_name = "Supply high";
    m_supplyPressureLow.m_name = "Supply low";
    m_addRemoveQuartile.setSupplyPressurePtr(&m_supplyPressureHigh, &m_supplyPressureLow);
    dataSource.setSupplyPressurePtr(&m_supplyPressureHigh, &m_supplyPressureLow);
    QVector<Valve*> valveList = {&vAR1, &vAR2, &vAR3};
    m_addRemoveQuartile.addValvePtrs(valveList);
}

void Grams::initStorageQuartile(){
    QVector<Valve*> valvesList = {&vS1, &vS2, &vS3, &vS4};
    m_storageQuartile.addValvePtrs(valvesList);
    m_storageQuartile.setIndexValveRange(3); // vS4
    m_storageQuartile.setIndexPressureHighLow(0, 1); // prSH, prSA
    m_storageQuartile.setIndexTemperatureMain(0); // tmSK
    const auto& storage_names = m_quartileManager.fillStorageQuartile(m_storageQuartile);
    QVector<MolesData*> molesDataList = {&mlB, &mlSC1, &mlSC2, &mlSC3, &mlD1};
    for(int i = 0; i < molesDataList.count(); ++i){
        molesDataList[i]->m_name = storage_names[i];
    }
    QVector<ControllerData*> pressureSensorsList = {&prSH, &prSA, &prSK};
    m_storageQuartile.addPressurePtrs(pressureSensorsList);
    QVector<ControllerData*> temperatureSensorsList = {&tmSK, &tmS, &tmSLittle, &tmSSmall, &tmSLarge};
    m_storageQuartile.addTemperaturePtrs(temperatureSensorsList);
    prSC1.m_name = "prSC1";
    prSC2.m_name = "prSC2";
    prSC3.m_name = "prSC3";
    QVector<DataCollection*> cVolumeSensorsList = {&prSC1, &prSC2, &prSC3};
    m_storageQuartile.setQuartileDataPressure(&prSQ);
    m_storageQuartile.setQuartileDataTemperature(&tmSQ);
    m_storageQuartile.setCVolumePtr(cVolumeSensorsList);
    m_storageQuartile.setBD1VolumePtr(&prSB, &prSD1);
    m_storageQuartile.setMolesPtr(molesDataList);
    QString baseNode = storage_names[0];
    for(int i = 1; i < 5; ++i){
        QString addNode = storage_names[i];
        m_storageQuartile.addPressureNode(baseNode+addNode, baseNode, addNode);
    }
    // test
    m_storageQuartile.updateQuartileData();
}

void Grams::setValveState(bool state, int index){
    // signal from GUI to change state of object
    Valve *valveList[16] = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    Valve *valve = valveList[index];
    const bool originalState = valveList[index]->getState();
    bool safe_state = m_safeModule.checkValveAction(valve->m_name, state);
    valve->setState(safe_state);
        if(!dataSource.setValveStates())
            valve->setState(originalState);
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
    qmlRegisterType<LightPlotItem>("LightPlot", 1, 0, "LightPlotItem");

    qmlRegisterSingletonInstance("Grams.dataSourceSingleton", 1, 0, "DataSource", &dataSource);

    qmlRegisterSingletonInstance("Grams.addRemoveQuartileSingleton", 1, 0, "AddRemoveQuar", &m_addRemoveQuartile);

    m_engine.rootContext()->setContextProperty("initSource", &initSource); // make singleton later
    //m_engine.rootContext()->setContextProperty("openGLSupported", openGLSupported);
    // m_engine.rootContext()->setContextProperty("_valveModel", &valveModel);
    // m_engine.rootContext()->setContextProperty("_myModel", &dataModel);
    m_engine.rootContext()->setContextProperty("safeModule", &m_safeModule);
    qmlRegisterSingletonInstance("Grams.backendSourceSingleton", 1, 0, "Grams", this);

    m_engine.load(url);

    // m_benchmarkTime.start();
}

void Grams::getCustomPlotPtr(CustomPlotItem* customPlotPointer){
    m_testPlot = customPlotPointer;
    QVector<DataCollection*> chartPtrs;
    chartPtrs.append(&prSH);
    chartPtrs.append(&prSA);
    m_testPlot->setDataPointers(&timeAnalog, chartPtrs);
    m_testPlot->initCustomPlot();
    m_testPlot->placeGraph();
    m_testPlot->dataUpdated();
    // connect(&testController, &TestController::valueChanged, m_testAxisTag, &CustomPlotItem::dataUpdated);
    // connect(&analogController, &IcpAICtrl::valueChanged, m_testAxisTag, &CustomPlotItem::dataUpdated);
}

int Grams::getFilterPlotPtr(CustomPlotItem* filterPlotPointer){
    QVector<DataCollection*> chartPtrs;
    switch(m_filterPlots.count()){
        case 0:
        {
            chartPtrs.append(&fl_prSH);
            filterPlotPointer->setDataPointers(&timeFilter, chartPtrs);
            break;
        }
        case 1:
        {
            chartPtrs.append(&fl_prSA);
            filterPlotPointer->setDataPointers(&timeFilter, chartPtrs);
            break;
        }
        default:
            qDebug() << "default\n"; // no error
            break;
    }
    filterPlotPointer->initCustomPlot();
    filterPlotPointer->placeGraph();
    filterPlotPointer->dataSetUpdated();
    m_filterPlots << filterPlotPointer;
    return m_filterPlots.count() - 1;
}

void Grams::initSafeModule(){
    m_safeModule.constructValveMap(initSource.m_hardware.m_valves);
    Valve *valveList[16] = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    for(int i = 0; i < 16; i++){
        m_safeModule.setInitialState(valveList[i]->m_name, valveList[i]->getState());
    }
    m_safeModule.setContradictionValves(initSource.m_security.m_contradictionValves);
    m_safeModule.setRuleOfThreeValves(initSource.m_security.m_twoOfThree);
    
    m_safeModule.setRangePressureValves(initSource.m_storageQuar.m_pressureRangeValve,"storageQuar",
        initSource.m_storageQuar.m_pressureRange_open,initSource.m_storageQuar.m_pressureRange_close);
    m_safeModule.setRangePressureValves(initSource.m_reactionQuar.m_pressureRangeValve,"reactionQuar",
        initSource.m_reactionQuar.m_pressureRange_open,initSource.m_reactionQuar.m_pressureRange_close);
    m_safeModule.setSafeReleaseValves(initSource.m_storageQuar.m_gasReleaseValve, "storageQuar", 
        initSource.m_storageQuar.m_gasRelease);
    
    m_safeModule.setGasSupplyValves(initSource.m_addRemoveQuar.m_gasSupplyValves);
    m_safeModule.setGasLeakageValves(initSource.m_reactionQuar.m_gasLeakageValves);
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
    m_tempVals.g_tmX = tmX.getCurValue();
    m_tempVals.g_tmY = tmY.getCurValue();
    m_tempVals.g_tmSLittle = tmSLittle.getCurValue();
    m_tempVals.g_tmSSmall = tmSSmall.getCurValue();
    m_tempVals.g_tmSLarge = tmSLarge.getCurValue();
    m_tempVals.g_tmSTube = tmSTube.getCurValue();
    m_tempVals.g_tmRTube = tmRTube.getCurValue();
    m_tempVals.g_tmF = tmF.getCurValue();
    emit guiValsTempChanged();
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
    guiValsUpdate();
    m_testPlot->dataUpdated();
    m_filterPlots[0]->dataSetUpdated();
    m_filterPlots[1]->dataSetUpdated();
    // additional checks

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

guiValsPres Grams::getGuiValsPres() const{
    return m_pressureVals;
}

guiValsTemp Grams::getGuiValsTemp() const{
    return m_tempVals;
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