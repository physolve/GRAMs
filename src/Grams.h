#pragma once

#include <QApplication>
#include <QQmlApplicationEngine>

#include "DataAcquisition.h"
// #include "DataModel.h"
#include "ValveModel.h"
#include "Initialize.h"
#include "Security.h"
#include "DataCollection.h"
#include "addon/Quartile.h"
#include "addon/StorageQuartile.h"
#include "addon/AddRemoveQuartile.h"
#include "addon/ReactionQuartile.h"
#include "addon/SecondLineQuartile.h"

#include "addon/QuartileManager.h"
#include "CustomPlotItem.h"
#include "addon/LightPlotItem.h"
#include "measure/Chamber.h"

#include "db/PostgreDB.h"
#include "TimeStamp.h"
#include "addon/TestField.h"

struct guiValsPres{ // sample
    Q_GADGET
    // it might be linked to json for import and multi-result log
    Q_PROPERTY (double prSH     MEMBER g_prSH)
    Q_PROPERTY (double prSA     MEMBER g_prSA)
    Q_PROPERTY (double prRH     MEMBER g_prRH)
    Q_PROPERTY (double prRA     MEMBER g_prRA)
    Q_PROPERTY (double prRL     MEMBER g_prRL)
    Q_PROPERTY (double prSK     MEMBER g_prSK)
    Q_PROPERTY (double tmSK     MEMBER g_tmSK)
    Q_PROPERTY (double tmS      MEMBER g_tmS)
public:
    double g_prSH;  // bar
    double g_prSA;  // bar
    double g_prRH;  // bar
    double g_prRA;  // bar
    double g_prRL;  // bar
    double g_prSK;  // bar
    double g_tmSK;  // bar
    double g_tmS;   // bar
};

struct guiValsTemp{ 
    Q_GADGET
    Q_PROPERTY (double tmX          MEMBER g_tmX)
    Q_PROPERTY (double tmY          MEMBER g_tmY)
    Q_PROPERTY (double tmSLittle    MEMBER g_tmSLittle)
    Q_PROPERTY (double tmSSmall     MEMBER g_tmSSmall)
    Q_PROPERTY (double tmSLarge     MEMBER g_tmSLarge)
    Q_PROPERTY (double tmSTube      MEMBER g_tmSTube)
    Q_PROPERTY (double tmRTube      MEMBER g_tmRTube)
    Q_PROPERTY (double tmF          MEMBER g_tmF)
public:
    double g_tmX;
    double g_tmY;
    double g_tmSLittle;
    double g_tmSSmall;
    double g_tmSLarge;
    double g_tmSTube;
    double g_tmRTube;
    double g_tmF;
};

class Grams : public QApplication
{
    Q_OBJECT
    Q_PROPERTY (bool vAR1State      READ getVAR1State   NOTIFY valveChanged)
    Q_PROPERTY (bool vAR2State      READ getVAR2State   NOTIFY valveChanged)
    Q_PROPERTY (bool vAR3State      READ getVAR3State   NOTIFY valveChanged)
    Q_PROPERTY (bool vAR4State      READ getVAR4State   NOTIFY valveChanged)
    Q_PROPERTY (bool vAR5State      READ getVAR5State   NOTIFY valveChanged)
    Q_PROPERTY (bool vAR6State      READ getVAR6State   NOTIFY valveChanged)
    Q_PROPERTY (bool vS1State       READ getVS1State    NOTIFY valveChanged)
    Q_PROPERTY (bool vS2State       READ getVS2State    NOTIFY valveChanged)
    Q_PROPERTY (bool vS3State       READ getVS3State    NOTIFY valveChanged)
    Q_PROPERTY (bool vS4State       READ getVS4State    NOTIFY valveChanged)
    Q_PROPERTY (bool vR1State       READ getVR1State    NOTIFY valveChanged)
    Q_PROPERTY (bool vR2State       READ getVR2State    NOTIFY valveChanged)
    Q_PROPERTY (bool vR3State       READ getVR3State    NOTIFY valveChanged)
    Q_PROPERTY (bool vR4State       READ getVR4State    NOTIFY valveChanged)
    Q_PROPERTY (bool vR5State       READ getVR5State    NOTIFY valveChanged)
    Q_PROPERTY (bool vSL1State      READ getVSL1State   NOTIFY valveChanged)
    Q_PROPERTY (bool vSL2State      READ getVSL2State   NOTIFY valveChanged)
    Q_PROPERTY (guiValsPres guiPres READ getGuiValsPres NOTIFY guiValsPresChanged)
    Q_PROPERTY (guiValsTemp guiTemp READ getGuiValsTemp NOTIFY guiValsTempChanged)
    Q_PROPERTY (guiValsPresVirtual guiPresVirtual READ getGuiPresVirtual NOTIFY guiPresVirtualChanged)
public:
    Grams(int &argc, char **argvm, const QString &curInitProfile);
    ~Grams();
    Q_INVOKABLE void setValveState(bool state, int valveId);
    Q_INVOKABLE void setManualChamberValve(bool state);
    Q_INVOKABLE void getCustomPlotPtr(CustomPlotItem* customPlotPointer);
    Q_INVOKABLE int getFilterPlotPtr(CustomPlotItem* customPlotPointer);

    Q_INVOKABLE void chamberSetUp();
    Q_INVOKABLE void refreshTestField();
signals:
    void valveChanged();
    void guiValsPresChanged();
    void guiValsTempChanged();
    void guiPresVirtualChanged();

private slots:
    void softEvent();

private:
    void initDigitalData();
    void initAnalogData();
    void advDoController();
    void advAiController();
    void vacuumController();
    void initGUI();
    void initSafeModule();
    void initAddRemoveQuartile();
    void initStorageQuartile();
    void initReactionQuartile();
    
    void initTimeStamp();
    void initTestField();
    
    void guiValsUpdate();
    void valveChangeUpdater(const QString& valveName);

    Initialize initSource;
    QQmlApplicationEngine m_engine;
    DataAcquisition dataSource; // pass from constructor
    // ValveModel valveModel;
    QTimer* softTimer;  // unique
    // MyModel dataModel;
    Security m_safeModule; // naming?

    // those are required GRAM50
    Valve vAR1; // 0 k104
    Valve vAR2; // 1 k109
    Valve vAR3; // 2 k114
    Valve vAR4; // 3 k118
    Valve vAR5; // 4 k178
    Valve vSL2; // 5 k192
    Valve vAR6; // 6 k176
    Valve vSL1; // 7 k179
    Valve vS4;  // 8 k171
    Valve vS1;  // 9 k131
    Valve vS2;  // 10 k133
    Valve vS3;  // 11 k135
    Valve vR1;  // 12 k153 (medium)
    Valve vR2;  // 13 k155 (slow)
    Valve vR3;  // 14 k151
    Valve vR4;  // 15 k173 
    Valve vR5; // chamber manual (config)

    bool getVAR1State() const;
    bool getVAR2State() const;
    bool getVAR3State() const;
    bool getVAR4State() const;
    bool getVAR5State() const;
    bool getVAR6State() const;
    bool getVS1State() const;
    bool getVS2State() const;
    bool getVS3State() const;
    bool getVS4State() const;
    bool getVR1State() const;
    bool getVR2State() const;
    bool getVR3State() const;
    bool getVR4State() const;
    bool getVR5State() const;
    bool getVSL1State() const;
    bool getVSL2State() const;

    ControllerData timeAnalog;

    ControllerData prSH;    // 0 DD311
    ControllerData prSA;    // 1 DD312
    ControllerData prRH;    // 2 DD331
    ControllerData prRA;    // 3 DD332
    ControllerData prRL;    // 4 DD334
    ControllerData prSK;    // 5 DD341
    ControllerData tmSK;    // 6 DT341
    ControllerData tmS;     // 7 DT314

    ControllerData tmX;         // 0 DT350 
    ControllerData tmY;         // 1 DT351
    ControllerData tmSLittle;   // 2 DT352
    ControllerData tmSSmall;    // 3 DT354
    ControllerData tmSLarge;    // 4 DT356
    ControllerData tmSTube;     // 5 DT357
    ControllerData tmRTube;     // 6 DT358
    ControllerData tmF;         // 7 DT359

    DataCollection prVac; // name?

    CustomPlotItem* m_testPlot;  // unique
    // make it QList
    guiValsPres m_pressureVals;
    guiValsPres getGuiValsPres() const;
    guiValsTemp m_tempVals;
    guiValsTemp getGuiValsTemp() const;
    guiValsPresVirtual m_guiPresVirtual;
    guiValsPresVirtual getGuiPresVirtual() const;
    
    // filters
    CustomPlotItem* m_mainPlot;  // unique
    QList<CustomPlotItem*> m_filterPlots;  // unique
    FilterData timeFilter;
    FilterData fl_prSH;    // 0 DD311
    FilterData fl_prSA;    // 1 DD312
    FilterData fl_prRH;    // 2 DD331
    FilterData fl_prRA;    // 3 DD332
    FilterData fl_prRL;    // 4 DD334
    FilterData fl_prSK;    // 5 DD341
    FilterData fl_tmSK;    // 6 DT341
    FilterData fl_tmS;     // 7 DT314
    QVector<FilterData*> getFilterPointers() {  // unique
        QVector<FilterData*> pointers;
        pointers.append(&fl_prSH);
        pointers.append(&fl_prSA);
        pointers.append(&fl_prRH);
        pointers.append(&fl_prRA);
        pointers.append(&fl_prRL);
        pointers.append(&fl_prSK);
        pointers.append(&fl_tmSK);
        pointers.append(&fl_tmS);
        return pointers;
    }
    // quartiles
    QuartileManager m_quartileManager;

    AddRemoveQuartile m_addRemoveQuartile;
    FilterData m_supplyPressureHigh;
    FilterData m_supplyPressureLow;
    DataCollection m_vacuumSensor;
    StorageQuartile m_storageQuartile;
    QuartileData prSQ;
    QuartileData tmSQ;
    DataCollection prSC1;
    MolesData mlSC1;
    DataCollection prSC2;
    MolesData mlSC2;
    DataCollection prSC3;
    MolesData mlSC3;
    DataCollection prSB;
    MolesData mlB;
    DataCollection prSD1;
    MolesData mlD1;
    
    ReactionQuartile m_reactionQuartile;
    FilterData m_reactionPressureHigh;
    FilterData m_reactionPressureLow;
    QuartileData prRQ;
    QuartileData tmRQ;
    DataCollection prRE;
    MolesData mlE;
    DataCollection prRD2Atm; // ?
    DataCollection prRD2Low; // ? rename to one
    MolesData mlD2;
    Chamber m_chamber;
    DataCollection prRF;
    MolesData mlF; // EF?

    SecondLineQuartile m_secondLineQuartile; 

    TimeStamp m_timeStamp;
    TestField m_testField;

    QElapsedTimer m_benchmarkTime;
};