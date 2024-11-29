#pragma once

#include <QVariant>

struct hardwareParameters{
    Q_GADGET
    Q_PROPERTY (QStringList                         valves          MEMBER m_valves)
    Q_PROPERTY (QMap<QString,QMap<QString,double>>  pressureSensors MEMBER m_pressureSensors)
    Q_PROPERTY (QStringList                         tempSensors     MEMBER m_tempSensors)
public:
    QStringList                             m_valves;
    QMap<QString,QMap<QString,double>>      m_pressureSensors;
    QStringList                             m_tempSensors;
};

struct daqParameters{
    Q_GADGET
    Q_PROPERTY (QString device                  MEMBER m_device)
    Q_PROPERTY (QString purpose                 MEMBER m_purpose)
    Q_PROPERTY (QString profile                 MEMBER m_profile)
    Q_PROPERTY (QString defaultType             MEMBER m_defaultType)
public:
    QString                                 m_device;
    QString                                 m_purpose;
    QString                                 m_profile;
    QString                                 m_defaultType;
};

struct addRemoveQuarParameters{
    Q_GADGET
    Q_PROPERTY (QStringList     gasSupplyValves     MEMBER m_gasSupplyValves)
    Q_PROPERTY (QStringList     gasDrainValves      MEMBER m_gasDrainValves)
    Q_PROPERTY (QString         vacuumSensor        MEMBER m_vacuumSensor)
public:
    QStringList                             m_gasSupplyValves;
    QStringList                             m_gasDrainValves;
    QString                                 m_vacuumSensor;
};

struct storageQuarParameters{
    Q_GADGET
    Q_PROPERTY (QStringList     gasStoreValves      MEMBER m_gasStoreValves)
    Q_PROPERTY (QString         gasReleaseValve     MEMBER m_gasReleaseValve)
    Q_PROPERTY (QString         pressureRangeValve  MEMBER m_pressureRangeValve)
    Q_PROPERTY (QStringList     highPressureSensors MEMBER m_highPressureSensors)
    Q_PROPERTY (QString         lowPressureSensor   MEMBER m_lowPressureSensor)
    Q_PROPERTY (QStringList     temperatureSensors  MEMBER m_temperatureSensors)
    Q_PROPERTY (double          pressureRange_close MEMBER m_pressureRange_close)
    Q_PROPERTY (double          pressureRange_open  MEMBER m_pressureRange_open)
    Q_PROPERTY (double          gasRelease          MEMBER m_gasRelease)
public:
    QStringList                             m_gasStoreValves;
    QString                                 m_gasReleaseValve;
    QString                                 m_pressureRangeValve;
    QStringList                             m_highPressureSensors;
    QString                                 m_lowPressureSensor;
    QStringList                             m_temperatureSensors;
    double                                  m_pressureRange_close;
    double                                  m_pressureRange_open;
    double                                  m_gasRelease;
};

struct reactionQuarParameters{
  Q_GADGET
    Q_PROPERTY (QStringList     gasLeakageValves    MEMBER m_gasLeakageValves)
    Q_PROPERTY (QString         gasReleaseValve     MEMBER m_gasReleaseValve)
    Q_PROPERTY (QString         pressureRangeValve  MEMBER m_pressureRangeValve)
    Q_PROPERTY (QString         highPressureSensor  MEMBER m_highPressureSensor)
    Q_PROPERTY (QStringList     lowPressureSensors  MEMBER m_lowPressureSensors)
    Q_PROPERTY (QStringList     temperatureSensors  MEMBER m_temperatureSensors)
    Q_PROPERTY (double          pressureRange_close MEMBER m_pressureRange_close)
    Q_PROPERTY (double          pressureRange_open  MEMBER m_pressureRange_open)
    Q_PROPERTY (double          gasRelease          MEMBER m_gasRelease)
public:
    QStringList                 m_gasLeakageValves;
    QString                     m_gasReleaseValve;
    QString                     m_pressureRangeValve; // possible second pressureRange
    QString                     m_highPressureSensor;
    QStringList                 m_lowPressureSensors;
    QStringList                 m_temperatureSensors;
    double                      m_pressureRange_close; // possible second condition
    double                      m_pressureRange_open; // possible second condition
    double                      m_gasRelease;
};

struct secondLineQuarParameters{
  Q_GADGET
    Q_PROPERTY (QString         gasLeakageValve     MEMBER m_gasLeakageValve)
    Q_PROPERTY (QString         mass_spectr         MEMBER m_mass_spectr)
public:
    QString                     m_gasLeakageValve; //?
    QString                     m_mass_spectr;
};

struct securityParameters{
  Q_GADGET
    Q_PROPERTY (QMap<QString, QStringList>  contradictionValves MEMBER m_contradictionValves)
    Q_PROPERTY (QString                     twoOfThree     MEMBER m_twoOfThree)
    Q_PROPERTY (QMap<QString, QStringList>  safetyQuars MEMBER m_safetyQuars)
public:
    QMap<QString, QStringList>  m_contradictionValves;
    QString                     m_twoOfThree;
    QMap<QString, QStringList>  m_safetyQuars;
};

class Initialize : public QObject
{
    Q_OBJECT
public:
    Initialize(QObject *parent = 0);
    Q_PROPERTY(QVariantMap profileJson MEMBER m_profileJson CONSTANT)
    Q_PROPERTY(QStringList profileNames MEMBER m_profileNames CONSTANT)
    Q_PROPERTY(QStringList advantechDeviceMap MEMBER m_advantechDeviceMap NOTIFY advantechDeviceMapChanged)
    Q_INVOKABLE QVariantMap advantechDeviceFill(const QString &description, const QString &type);
    
signals:
    void advantechDeviceMapChanged();
    //void advantechDeviceSettingsChanged();
    
private:
    bool readProfile(QString &rawData);
    bool jsonParser(QString &rawData);
    bool advantechDeviceCheck();

    QVariantMap m_profileJson;

    QStringList m_profileNames;

    QStringList m_advantechDeviceMap;

    //QVariantMap m_advantechDeviceSettings;

    void visualRepresentation();

    hardwareParameters hardware;
    daqParameters daq;
    addRemoveQuarParameters addRemoveQuar;
    storageQuarParameters storageQuar;
    reactionQuarParameters reactionQuar;
    secondLineQuarParameters secondLineQuar;
    securityParameters security;
};