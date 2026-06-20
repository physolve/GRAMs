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
    m_safeModule(), // check
    softTimer(new QTimer)
    /*
    , // unique pointer
    m_testPlot(nullptr)
    */
{
    initDigitalData();
    initAnalogData();
    advDoController();
    advAiController();
    vacuumController();

    initAddRemoveQuartile();
    initStorageQuartile();
    initReactionQuartile();

    initTimeStamp();
    initTestField();
    initDatabase();
    initPlayPressure();
    initActionHandler();

    initCharts();

    initGUI();
    initSafeModule();
    connect(this, &Grams::aboutToQuit, this, &Grams::beforeQuitting);
    connect(softTimer, &QTimer::timeout, this, &Grams::softEvent);
    softTimer->setInterval(500);
    if(initSource.isInitializeOk()){ // dataSource getGRAMsIntegrity ?
        softTimer->start();
    }
    dataSource.startAcquisition();
}

Grams::~Grams(){
    delete m_mainPlot;
    qDeleteAll(m_graphs);
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

    vR1.m_name = "R1"; // "Leakage medium";
    vR2.m_name = "R2"; // "Leakage slow";
    vR3.m_name = "R3"; // "Leakage tube";
    vR4.m_name = "R4"; // "Pressure range reaction";
    vR5.m_name = "R5"; // "Chamber manual valve";

    vSL1.m_name = "SL1"; // "Second line outlet";
    vSL2.m_name = "SL2"; // "Second line barrel";

    timeAnalog.m_name = "Время";

    QVector<double> indexData;
    for(int i = 0; i < Constants::filterPointCount; ++i) { //m_sectionLength
        indexData << i;
    }
    timeFilter.setData(indexData);
    for(auto filtersData : getFilterPointers()){ // ?
        filtersData->setData(QVector<double>(Constants::filterPointCount,0.0));
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
        pressureSensorsList[i]->m_type = DataType::Pressure;
        pressureSensorsList[i]->setCoeffs(pressureSensor.m_A/pressureSensor.m_R*1000.0, pressureSensor.m_B); //*1000.0 fix profile later
    }
    const auto& tempSensors = initSource.getTempSensors();
    for(int i = 0; i < 8; ++i){
        tempSensorsList[i]->m_name = tempSensors[i];
        tempSensorsList[i]->m_type = DataType::Temperature;
        tempSensorsList[i]->setCoeffs(1.0, 0.0);
    }
    // mole Volume names from addons js
    m_pressureVals = guiValsPres{0,0,0,0,0,0,0,0};
    m_tempVals = guiValsTemp{0,0,0,0,0,0,0,0};
}

void Grams::advDoController(){
    // pointers to valves
    QVector<Valve*> valveList = {&vAR1, &vAR2, &vAR3, &vAR4, &vAR5, &vSL2, &vAR6, &vSL1, &vS4, &vS1, &vS2, &vS3, &vR1, &vR2, &vR3, &vR4};
    m_valveControl.setValvePointers(valveList);
    m_valveControl.setChamberValvePointer(&vR5);
    m_valveControl.setAddRemoveQuartile(&m_addRemoveQuartile);
    m_valveControl.setReactionQuartile(&m_reactionQuartile);
    m_valveControl.setDatabase(&m_gramStateDB);
    m_valveControl.setSafeModule(&m_safeModule);
    m_valveControl.setSafeModuleInitialValveState();
    m_valveControl.setGasSupplyValves(initSource.m_addRemoveQuar.m_gasSupplyValves);
    m_valveControl.setGasStoreValves(initSource.m_storageQuar.m_gasStoreValves);

    if(!initSource.isInitializeOk())
        return;
    daqParameters parametersDO;
    initSource.getParametersDO(parametersDO);    
    // dataSource. set Required
    m_valveControl.initDaqDO(parametersDO);
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

void Grams::vacuumController(){
    m_vacuumSensor.m_name = "Вакууметр";
    m_vacuumSensor.m_type = DataType::Pressure;

    if(!initSource.isInitializeOk())
        return;
    
    const auto& parametersVacuum = initSource.getVacuumParameters();
    m_vacuumSensor.setAltUnitCoef(0.001333); // torr to bar
    dataSource.setVacuumPointer(&m_vacuumSensor);
    dataSource.initSerialVacuum(parametersVacuum);
    // update vacuum values
}

void Grams::initAddRemoveQuartile(){
    m_supplyPressureHigh.m_name = "Supply high";
    m_supplyPressureHigh.m_type = DataType::Pressure;
    m_supplyPressureLow.m_name = "Supply low";
    m_supplyPressureLow.m_type = DataType::Pressure;

    m_addRemoveQuartile.setSupplyPressurePtr(&m_supplyPressureHigh, &m_supplyPressureLow);
    m_addRemoveQuartile.setStorageQuartilePtr(&m_storageQuartile);
    
    m_addRemoveQuartile.setStorageQuartilePressure(&prSQ);
    m_addRemoveQuartile.setStorageQuartileTemperature(&tmSQ);
    dataSource.setSupplyPressurePtr(&m_supplyPressureHigh, &m_supplyPressureLow);

    QVector<Valve*> valveList = {&vAR1, &vAR2, &vAR3}; // drain ptr?
    m_addRemoveQuartile.addValvePtrs(valveList);
}

void Grams::initStorageQuartile(){
    QVector<Valve*> valvesList = {&vS1, &vS2, &vS3, &vS4};
    m_storageQuartile.addValvePtrs(valvesList);
    m_storageQuartile.setIndexValveRange(3); // vS4
    const auto& storage_names = m_quartileManager.fillStorageQuartile(&m_storageQuartile);
    QVector<MolesData*> molesDataList = {&mlB, &mlSC1, &mlSC2, &mlSC3, &mlD1};
    for(int i = 0; i < molesDataList.count(); ++i){
        molesDataList[i]->m_name = storage_names[i];
    }
    QVector<ControllerData*> pressureSensorsList = {&prSH, &prSA, &prSK};
    m_storageQuartile.addPressurePtrs(pressureSensorsList);
    QVector<ControllerData*> temperatureSensorsList = {&tmSK, &tmS, &tmSLittle, &tmSSmall, &tmSLarge};
    m_storageQuartile.addTemperaturePtrs(temperatureSensorsList);
    m_storageQuartile.setIndexPressureHighLow(0, 1); // prSH, prSA
    m_storageQuartile.setIndexTemperatureMain(0); // tmSK
    prSC1.m_name = "prSC1";
    prSC2.m_name = "prSC2";
    prSC3.m_name = "prSC3";
    QVector<DataCollection*> cVolumeSensorsList = {&prSC1, &prSC2, &prSC3};
    prSQ.m_name = "Эталонный давл.";
    prSQ.m_type = DataType::Pressure;
    m_storageQuartile.setQuartileDataPressure(&prSQ);
    tmSQ.m_name = "Эталонный темп.";
    tmSQ.m_type = DataType::Temperature;
    m_storageQuartile.setQuartileDataTemperature(&tmSQ);
    m_storageQuartile.setCVolumePtr(cVolumeSensorsList);
    m_storageQuartile.setBD1VolumePtr(&prSB, &prSD1);
    m_storageQuartile.setMolesPtr(molesDataList);
    QString baseNode = storage_names[0]; // 
    for(int i = 1; i < storage_names.count(); ++i){
        QString addNode = storage_names[i];
        m_storageQuartile.addPressureNode(baseNode+addNode, baseNode, addNode);
        // base nodes before changing
    }
    // test
    m_storageQuartile.updateQuartileData();
}

void Grams::initReactionQuartile(){
    m_reactionPressureHigh.m_name = "Reaction high";
    m_reactionPressureLow.m_name = "Reaction low";
    m_reactionQuartile.setReactionPressurePtr(&m_reactionPressureHigh, &m_reactionPressureLow);
    m_reactionQuartile.setStorageQuartilePtr(&m_storageQuartile);
    m_reactionQuartile.setStorageQuartilePressure(&prSQ);
    dataSource.setLeakagePressurePtr(&m_reactionPressureHigh, &m_reactionPressureLow);

    QVector<Valve*> valvesList = {&vR1, &vR2, &vR3, &vR4};
    m_reactionQuartile.addValvePtrs(valvesList);
    m_reactionQuartile.setIndexValveRange(3); // vR4
    
    const auto& storage_names = m_quartileManager.fillReactionQuartile(&m_reactionQuartile);

    QVector<MolesData*> molesDataList = {&mlE, &mlD2}; // chamber F as option
    for(int i = 0; i < molesDataList.count(); ++i){
        molesDataList[i]->m_name = storage_names[i];
    }
    
    QVector<ControllerData*> pressureSensorsList = {&prRH, &prRA, &prRL};
    m_reactionQuartile.addPressurePtrs(pressureSensorsList);
    QVector<ControllerData*> temperatureSensorsList = {&tmRTube, &tmF};
    m_reactionQuartile.addTemperaturePtrs(temperatureSensorsList);
    m_reactionQuartile.setIndexPressureHighLow(0, 1, 2); // prRH, prRA
    m_reactionQuartile.setIndexTemperatureMain(0); // tmSK
    prRQ.m_name = "Реакционный давл.";
    prRQ.m_type = DataType::Pressure;
    m_reactionQuartile.setQuartileDataPressure(&prRQ);
    tmRQ.m_name = "Реакционный темп.";
    tmRQ.m_type = DataType::Temperature;
    m_reactionQuartile.setQuartileDataTemperature(&tmRQ);
    m_reactionQuartile.setED2VolumePtr(&prRE, &prRD2Atm, &prRD2Low);
    m_reactionQuartile.setMolesPtr(molesDataList);
    QString baseNode = storage_names[0];
    for(int i = 1; i < storage_names.count(); ++i){ // chamber Node
        QString addNode = storage_names[i];
        m_reactionQuartile.addPressureNode(baseNode+addNode, baseNode, addNode);
        // base nodes before changing
    }
    // chamber options
    m_reactionQuartile.setChamberPointer(&m_chamber);
    m_reactionQuartile.setFVolumePtr(&prRF);
    m_reactionQuartile.setIndexTemperatureChamber(1); // tmF
    // test
    this->chamberSetUp();
    // chamber options
    m_reactionQuartile.updateQuartileData();
}

void Grams::initCharts(){
    m_mainPlot = new TwoAxisPlot(); // two axis plot Child
    QVector<DataCollection*> chartPtrs;
    chartPtrs.append(&prSQ);
    chartPtrs.append(&prRQ);
    chartPtrs.append(&tmRQ);
    chartPtrs.append(&tmF);
    m_mainPlot->setDataPointers(&timeAnalog, chartPtrs);
    m_mainPlot->setPlotColor();
    m_mainPlot->initPlot();
    m_mainPlot->placeLegend();
    m_mainPlot->update();
    m_mainPlot->dataUpdated();
    m_mainPlot->m_chartName = "Главный график";
    // additional charts
    // addGraph("vaccumChart");
    auto vacuumChart = new BasePlot(); // use Alt unit
    vacuumChart->setDataPointers(&timeAnalog, &m_vacuumSensor);
    vacuumChart->setPlotColor();
    vacuumChart->initPlot();
    vacuumChart->placeLegend();
    vacuumChart->setLogValueAxis();
    vacuumChart->setPrefferedAltUnit(true);
    vacuumChart->update();
    vacuumChart->dataUpdated();
    vacuumChart->m_chartName = "Вакуум";
    m_graphs << vacuumChart;
    // addGraph("secondChart");
}

void Grams::initGUI(){
    QQuickStyle::setStyle("Material");
    QString applicationName = "GRAMs"; // curInitProfile also?
    QLocale::setDefault(QLocale::c()); 
    // m_engine.addImportPath(":/");
    m_engine.addImportPath(":/modules/RuntableLib");
    const QUrl url(QString("qrc:/%1/qml/Main.qml").arg(applicationName));
    QObject::connect(
        &m_engine, &QQmlApplicationEngine::objectCreated, this,
        [url](QObject *obj, const QUrl &objUrl) {
            if(!obj && url == objUrl) QCoreApplication::exit(-1);
    },
    Qt::QueuedConnection);
    
    qmlRegisterType<BasePlot>("BasePlot", 1, 0, "BasePlotItem");
    qRegisterMetaType<Condition>();
    qmlRegisterUncreatableMetaObject(RegimeEnums::staticMetaObject, "com.grams.prototable", 1, 0, "RegimeState", "Error: only enums");
    
    qmlRegisterSingletonInstance("com.grams.prototable", 1, 0, "RegimeManager", &m_regimeManager);
    qmlRegisterSingletonInstance("Grams.initSourceSingleton", 1, 0, "InitSource", &initSource);
    
    qmlRegisterSingletonInstance("Grams.dataSourceSingleton", 1, 0, "DataSource", &dataSource);
    qmlRegisterSingletonInstance("Grams.valveControlSingleton", 1, 0, "ValveControl", &m_valveControl);
    qmlRegisterSingletonInstance("Grams.addRemoveQuartileSingleton", 1, 0, "AddRemoveQuar", &m_addRemoveQuartile);
    qmlRegisterSingletonInstance("Grams.reactionQuartileSingleton", 1, 0, "ReactionQuar", &m_reactionQuartile);

    qmlRegisterSingletonInstance("Grams.actionHandlerSingleton", 1, 0, "ActionHandler", &m_actionHandler);
    qmlRegisterSingletonInstance("Grams.regimeTaskTreeSingleton", 1, 0, "RegimeTaskTree", &m_regimeTaskTree);
    //m_engine.rootContext()->setContextProperty("openGLSupported", openGLSupported);
    // m_engine.rootContext()->setContextProperty("_valveModel", &valveModel);
    // m_engine.rootContext()->setContextProperty("_myModel", &dataModel);
    // m_engine.rootContext()->setContextProperty("safeModule", &m_safeModule);
    
    qmlRegisterSingletonInstance("Grams.backendSourceSingleton", 1, 0, "Grams", this);
    qmlRegisterSingletonInstance("Grams.chamberChooserSingleton", 1, 0, "ChamberChooser", &m_chamber);
    qmlRegisterSingletonInstance("Grams.timeStampSingleton", 1, 0, "TimeStamp", &m_timeStamp);
    qmlRegisterSingletonInstance("Grams.testFieldSingleton", 1, 0, "TestFieldBack", &m_testField);
    qmlRegisterSingletonInstance("Grams.playPressureSingleton", 1, 0, "PlayPressure", &m_playPressure);
    m_engine.load(url);
}
/*
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
        case 2:
        {
            chartPtrs.append(&fl_prRH);
            filterPlotPointer->setDataPointers(&timeFilter, chartPtrs);
            break;
        }
        case 3:
        {
            chartPtrs.append(&fl_prRA);
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
*/

QStringList Grams::getChartNames() const{
    QStringList chartNames;
    for(auto graph : m_graphs){
        chartNames << graph->m_chartName;
    }
    return chartNames;
}

QList<BasePlot*> Grams::getGraphs() const
{
    return m_graphs;
}

Q_INVOKABLE void Grams::addGraph(const QString &key)
{
    // if(m_graphs.contains(key)) return;

    // auto g = m_customPlot->addGraph();
    // if(g == nullptr) return;
    // g->setName(key);
    // auto graph = new BasePlot();
    // m_graphs.insert(key, graph);
    auto graph = new BasePlot();

    m_graphs << graph;
    emit graphsChanged();
}

Q_INVOKABLE void Grams::removeGraph(const QString &key)
{
    // if(m_graphs.contains(key)) {
    //     auto graph = m_graphs.take(key);
    //     delete graph;
    //     emit graphsChanged();
    // }
    auto last = m_graphs.takeLast();
    delete last;
    emit graphsChanged();
}

void Grams::initSafeModule(){
    m_safeModule.constructValveMap(initSource.m_hardware.m_valves);
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

void Grams::initTimeStamp(){
    QVariantList initialTimeStamp;
    if(!GramStateDB::createConnection(initialTimeStamp)){
        // online or offline mode
        return;
    }
    // current id check
    // current time check
    // int id;
    if(initialTimeStamp.isEmpty()){
        return;
    }
    // unpredictable behavior
    int id = initialTimeStamp[0].toInt();
    QDateTime timeStamp = initialTimeStamp[1].toDateTime();
    guiValsPresVirtual pressureVals;
    pressureVals.g_prSQ = initialTimeStamp[2].toDouble();
    pressureVals.g_prRQ = initialTimeStamp[3].toDouble();
    pressureVals.g_prSC1 = initialTimeStamp[4].toDouble();
    pressureVals.g_prSC2 = initialTimeStamp[5].toDouble();
    pressureVals.g_prSC3 = initialTimeStamp[6].toDouble();
    pressureVals.g_prSB = initialTimeStamp[7].toDouble();
    pressureVals.g_prSD1 = initialTimeStamp[8].toDouble();
    pressureVals.g_prRE = initialTimeStamp[9].toDouble();
    pressureVals.g_prRD2 = initialTimeStamp[10].toDouble();
    pressureVals.g_prRF = initialTimeStamp[11].toDouble();
    m_timeStamp.setInitialPressure(timeStamp, pressureVals);
    prSQ.addPoint(initialTimeStamp[2].toDouble());
    tmSQ.addPoint(27);
    prRQ.addPoint(initialTimeStamp[3].toDouble());
    tmRQ.addPoint(27);
    prSC1.addPoint(initialTimeStamp[4].toDouble());
    prSC2.addPoint(initialTimeStamp[5].toDouble());
    prSC3.addPoint(initialTimeStamp[6].toDouble());
    prSB.addPoint(initialTimeStamp[7].toDouble());
    prSD1.addPoint(initialTimeStamp[8].toDouble());
    prRE.addPoint(initialTimeStamp[9].toDouble());
    prRD2Atm.addPoint(initialTimeStamp[10].toDouble());
    prRF.addPoint(initialTimeStamp[11].toDouble());
    guiValsUpdate();
    // check to real
}

void Grams::initTestField(){
    refreshTestField();
}

void Grams::refreshTestField(){
    m_storageQuartile.updateVolumeObjects();
    m_reactionQuartile.updateVolumeObjects();
    // m_testField.setStorageNodes(m_storageQuartile.getPressureNodes());
    // m_testField.setReactionNodes(m_reactionQuartile.getPressureNodes());
    m_testField.updateTestField(m_storageQuartile.getPressureNodes(), m_storageQuartile.getUsedVolumes()); // storage nodes +m_reactionQuartile.getUsedVolumes()
}

void Grams::initDatabase(){
    // user info
    if(m_gramStateDB.initDatabase()){
        // do something else
        //)
        QVector<DataCollection*> timeStampDataPointers = {&prSQ, &prRQ, &prSC1, &prSC2, &prSC3, &prSB, &prSD1, &prRE, &prRD2Atm, &prRF};
        m_gramStateDB.setTimeStampDataPointers(timeStampDataPointers);
        // if(m_gramStateDB.queryTimeStamp())
        //     qDebug() << "Query reserved"; // ok?
        // else
        //     qDebug() << "Query not reserved";
    }
}

void Grams::initPlayPressure(){
    m_playPressure.setARQ(&m_addRemoveQuartile);
    m_playPressure.setSQ(&m_storageQuartile);
    m_playPressure.setRQ(&m_reactionQuartile);
}

void Grams::initActionHandler(){
    m_actionHandler.setValveControl(&m_valveControl);
    m_actionHandler.setDataAcquisition(&dataSource);
    m_actionHandler.setSecurity(&m_safeModule);
    m_actionHandler.setAddRemoveQuartile(&m_addRemoveQuartile);

    // RegimeTaskTree — TaskTree-based regime orchestrator
    m_regimeTaskTree.setRegimeManager(&m_regimeManager);
    m_regimeTaskTree.setValveControl(&m_valveControl);
    m_regimeTaskTree.setDataAcquisition(&dataSource);
    m_regimeTaskTree.setSecurity(&m_safeModule);
}

void Grams::testActionHandler(){
    m_actionHandler.runInletAction();
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
    m_pressureVals.g_prARV = m_vacuumSensor.getAltUnit(); // bar
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
    m_guiPresVirtual.g_prSQ = prSQ.getCurValue();
    m_guiPresVirtual.g_prRQ = prRQ.getCurValue();
    m_guiPresVirtual.g_prSC1 = prSC1.getCurValue();
    m_guiPresVirtual.g_prSC2 = prSC2.getCurValue();
    m_guiPresVirtual.g_prSC3 = prSC3.getCurValue();
    m_guiPresVirtual.g_prSB = prSB.getCurValue();
    m_guiPresVirtual.g_prSD1 = prSD1.getCurValue();
    m_guiPresVirtual.g_prRE = prRE.getCurValue();
    m_guiPresVirtual.g_prRD2 = prRD2Atm.getCurValue();
    m_guiPresVirtual.g_prRF = prRF.getCurValue();
    emit guiPresVirtualChanged();
}

void Grams::softEvent(){
    guiValsUpdate();
    /*
    if(m_testPlot!=nullptr)
        m_testPlot->dataUpdated();
    if(!m_filterPlots.isEmpty()){
        for(auto plot : m_filterPlots){
            plot->dataSetUpdated(); 
        }
    }
    */
    m_mainPlot->dataUpdated();
    for(auto graph : m_graphs){
        graph->dataUpdated();
    }
    // additional checks
    m_storageQuartile.updateQuartileData();
    m_reactionQuartile.updateQuartileData();
}

void Grams::chamberSetUp(){
    VolumeObject chamber;
    chamber.name = "10-02";
    chamber.volume = 25.405;
    m_chamber.setChamberVolume(chamber); // rewrite Volume object for chamber to use in quartile with other
    m_chamber.setCraneToChamber(26.1327 - 25.7941);
    m_reactionQuartile.setChamber(chamber.name);
    m_valveControl.setManualChamberValve(true);
}

guiValsPres Grams::getGuiValsPres() const{
    return m_pressureVals;
}

guiValsTemp Grams::getGuiValsTemp() const{
    return m_tempVals;
}

guiValsPresVirtual Grams::getGuiPresVirtual() const{
    return m_guiPresVirtual;
}

void Grams::beforeQuitting(){
    // quitting
    // get this value from db settings table
    bool autoSave = true;
    if(autoSave){
        if(m_gramStateDB.writeTimeStamp())
            qDebug() << "Time Stamp has been written";
        else 
            qDebug() << "Time Stamp has not been written";
    }
}