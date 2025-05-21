#pragma once

#include "Quartile.h"

#include "StorageQuartile.h"

class AddRemoveQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit AddRemoveQuartile(QObject *parent = nullptr);
    virtual ~AddRemoveQuartile();
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    
    void setStorageQuartilePressure(QuartileData* storageQuartilePressure);
    void setStorageQuartilePtr(StorageQuartile* storageQuartile);
    void updatePortState();

    Q_INVOKABLE int getLightPlotPtr(LightPlotItem* customPlotPointer);
    Q_INVOKABLE void setSupplyAdjustParameters(QVariantMap parameters);
    Q_INVOKABLE void startSupplyMeasure(bool measure);

private slots:
    void expEvent();
private:
    void fillSupplyPortData();
    void preCalculateSupplyTime(int portId, double turn, double portPressure);
    double m_supplySpeed; // current
    double m_drainSpeed;
    int m_currentSupplyPort;
    SupplyPort m_supplyPort[3]; // settings different but process only one!
    // custom plot graph local
    FilterData* m_supplyPressureHigh;
    FilterData* m_supplyPressureLow;
    QList<LightPlotItem*> m_supplyPressurePlots;
    // quartile pressure from StorageQuartile
    QTimer* m_expUpdate;
    QuartileData* m_storageQuartilePressure;
    StorageQuartile* m_storageQuartile; // try more header files and just ask another
    
};