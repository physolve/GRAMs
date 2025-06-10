#pragma once

#include "Quartile.h"

#include "StorageQuartile.h"

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
public:
    explicit AddRemoveQuartile(QObject *parent = nullptr);
    virtual ~AddRemoveQuartile();
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    
    void setStorageQuartilePressure(QuartileData* storageQuartilePressure);
    void setStorageQuartilePtr(StorageQuartile* storageQuartile);
    void updatePortState();

    void setInletStrategy(const InletStrategy& inletStrategy);
    InletStrategy getInletStrategy() const;
    bool checkSupplyAction();
signals:
    void inletStrategyChanged();
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
    // QList<LightPlotItem*> m_supplyPressurePlots;
    // quartile pressure from StorageQuartile

    // QTimer* m_expUpdate;
    
    QuartileData* m_storageQuartilePressure;
    StorageQuartile* m_storageQuartile; // try more header files and just ask another

    InletStrategy m_inletStrategy;
};