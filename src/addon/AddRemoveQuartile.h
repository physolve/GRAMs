#pragma once

#include "Quartile.h"


class StorageQuartile;
class LightPlot;

struct InletStrategy{
    Q_GADGET
    Q_PROPERTY (double  reducerLimit    MEMBER m_reducerLimit)
    Q_PROPERTY (QString usePort         MEMBER m_usePort)
    Q_PROPERTY (double  pressureLimit   MEMBER m_pressureLimit)
    Q_PROPERTY (int     openTime        MEMBER m_openTime)
public:
    double  m_reducerLimit; // bar
    QString m_usePort; // [0-2]
    double  m_pressureLimit; // bar in storage
    int     m_openTime; // ms
};

// struct guiInletAction{
//     Q_GADGET
//     Q_PROPERTY (double stPresTg         MEMBER m_storagePressureTarget)
//     Q_PROPERTY (double c1PresTg         MEMBER m_c1PressureTarget)
//     Q_PROPERTY (double c2PresTg         MEMBER m_c2PressureTarget)
//     Q_PROPERTY (double c3PresTg         MEMBER m_c3PressureTarget)
//     Q_PROPERTY (double timeOpenGasPort  MEMBER m_timeOpenGasPort)
// public:
//     double m_storagePressureTarget;
//     double m_c1PressureTarget;
//     double m_c2PressureTarget;
//     double m_c3PressureTarget;
//     double m_timeOpenGasPort;
// };

class AddRemoveQuartile : public Quartile
{
    Q_OBJECT
    Q_PROPERTY(InletStrategy inletStrategy READ getInletStrategy WRITE setInletStrategy NOTIFY inletStrategyChanged)
    Q_PROPERTY(QList<LightPlot*> addRemoveGraphs READ getAddRemoveGraphs NOTIFY addRemoveGraphsChanged)
    Q_PROPERTY(QStringList addRemoveChartNames READ getAddRemoveChartNames NOTIFY addRemoveGraphsChanged)

public:
    explicit AddRemoveQuartile(QObject *parent = nullptr);
    virtual ~AddRemoveQuartile();
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    
    void setStorageQuartilePressure(QuartileData* storageQuartilePressure);
    void setStorageQuartileTemperature(QuartileData* storageQuartileTemperature);
    void setStorageQuartilePtr(StorageQuartile* storageQuartile);

    Q_INVOKABLE void initAddRemoveCharts();
    Q_INVOKABLE void clearAddRemoveCharts();

    void updatePortState();
    void setInletStrategy(const InletStrategy& inletStrategy);
    InletStrategy getInletStrategy() const;
    
    bool checkSupplyAction();
    void fillSupplyActionData(unsigned int nowTime);
    bool appendSupplyActionData(unsigned int nowTime);
    void saveSupplyActionData();
signals:
    void inletStrategyChanged();
    void addRemoveGraphsChanged();
private:
    void preCalculateSupplyTime(double nowTimeS, int portId, double start_pressure);
    double m_supplySpeed; // current
    double m_drainSpeed;
    // int m_currentSupplyPort;
    SupplyPort m_supplyPort[3]; // settings different but process only one!
    // custom plot graph local
    
    FilterData* m_supplyPressureHigh;
    FilterData* m_supplyPressureLow;
    
    QList<LightPlot*> m_addRemoveGraphs;
    QList<LightPlot*> getAddRemoveGraphs() const;
    QStringList getAddRemoveChartNames() const;
    // quartile pressure from StorageQuartile

    // QTimer* m_expUpdate;
    
    QuartileData* m_storageQuartilePressure;
    QuartileData* m_storageQuartileTemperature;
    StorageQuartile* m_storageQuartile; // try more header files and just ask another

    InletStrategy m_inletStrategy;
};