#pragma once

#include "Quartile.h"
#include "../charts/LightPlot.h"

class StorageQuartile;
// class LightPlot;

struct InletStrategy{
    Q_GADGET
    Q_PROPERTY (double  reducerLimit    MEMBER m_reducerLimit) // mutable
    Q_PROPERTY (QString usePort         MEMBER m_usePort) // mutable
    Q_PROPERTY (double  pressureLimit   MEMBER m_pressureLimit) // mutable
    Q_PROPERTY (int     openTime        MEMBER m_openTime) // mutable
public:
    double  m_reducerLimit;     // bar
    QString m_usePort;          // [0-2]
    double  m_pressureLimit;    // bar in storage
    int     m_openTime;         // ms
};


class AddRemoveQuartile : public Quartile
{
    Q_OBJECT
    Q_PROPERTY(InletStrategy inletStrategy READ getInletStrategy WRITE setInletStrategy NOTIFY inletStrategyChanged)
    Q_PROPERTY(QList<LightPlot*> addRemoveGraphs READ getAddRemoveGraphs NOTIFY addRemoveGraphsChanged)
    Q_PROPERTY(QStringList addRemoveChartNames READ getAddRemoveChartNames NOTIFY addRemoveGraphsChanged)
    Q_PROPERTY(QList<double> rateSupply READ getRateSupply NOTIFY rateSupplyChanged)
public:
    explicit AddRemoveQuartile(QObject *parent = nullptr);
    virtual ~AddRemoveQuartile();
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    
    void setStorageQuartilePressure(QuartileData* storageQuartilePressure);
    void setStorageQuartileTemperature(QuartileData* storageQuartileTemperature);
    void setStorageQuartilePtr(StorageQuartile* storageQuartile);

    Q_INVOKABLE void initAddRemoveCharts();
    Q_INVOKABLE void clearAddRemoveCharts();

    // void updatePortState();
    void setInletStrategy(const InletStrategy& inletStrategy);
    InletStrategy getInletStrategy() const;
    
    bool checkSupplyAction();
    void fillSupplyActionData(unsigned int nowTime);
    bool appendSupplyActionData(unsigned int nowTime);
    void saveSupplyActionData();
signals:
    void inletStrategyChanged();
    void addRemoveGraphsChanged();
    void rateSupplyChanged();
private:
    void preCalculateSupplyTime(int portId, double start_pressure);
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
    // quartile pressure from StorageQuartil

    QList<double> getRateSupply();
    
    QuartileData* m_storageQuartilePressure;
    QuartileData* m_storageQuartileTemperature;
    StorageQuartile* m_storageQuartile; // try more header files and just ask another

    InletStrategy m_inletStrategy;
};