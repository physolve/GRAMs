#pragma once

#include <QVariant>

struct PressureSensor{
    Q_GADGET
    Q_PROPERTY (QString sensorName MEMBER m_sensorName)
    Q_PROPERTY (int cch MEMBER m_cch)
    Q_PROPERTY (double A MEMBER m_A)
    Q_PROPERTY (double B MEMBER m_B)
    Q_PROPERTY (double R MEMBER m_R)
public:
    // PressureSensor() = default;
    QString m_sensorName;
    int m_cch;
    double m_A;
    double m_B;
    double m_R;
    // other parameters to easy get
    bool operator==(const PressureSensor& other) const = default;
};

struct hardwareParameters{
    Q_GADGET
    Q_PROPERTY (QStringList             valves          MEMBER m_valves)
    Q_PROPERTY (QStringList             tempSensors     MEMBER m_tempSensors)
    Q_PROPERTY (QList<PressureSensor>   pressureSensors MEMBER m_pressureSensors)
public:
    QStringList                             m_valves;
    QList<PressureSensor>                   m_pressureSensors;
    QStringList                             m_tempSensors;
};

struct daqParameters{
    Q_GADGET
    Q_PROPERTY (QString         device                  MEMBER m_device)
    Q_PROPERTY (QString         purpose                 MEMBER m_purpose)
    Q_PROPERTY (QString         profile                 MEMBER m_profile)
    Q_PROPERTY (int             defaultType             MEMBER m_defaultType)
    Q_PROPERTY (bool            state                   MEMBER m_state)
public:
    QString                     m_device;
    QString                     m_purpose;
    QString                     m_profile;
    int                         m_defaultType;
    bool                        m_state;
    QString                     fullName;
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
    Q_PROPERTY (QString         pressureRangeValve  MEMBER m_pressureRangeValve)
    Q_PROPERTY (QString         highPressureSensor  MEMBER m_highPressureSensor)
    Q_PROPERTY (QStringList     lowPressureSensors  MEMBER m_lowPressureSensors)
    Q_PROPERTY (QStringList     temperatureSensors  MEMBER m_temperatureSensors)
    Q_PROPERTY (double          pressureRange_close MEMBER m_pressureRange_close)
    Q_PROPERTY (double          pressureRange_open  MEMBER m_pressureRange_open)
    Q_PROPERTY (double          gasRelease          MEMBER m_gasRelease)
public:
    QStringList                 m_gasLeakageValves;
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
    Q_PROPERTY (QMap<QString, QStringList>  contradictionValves     MEMBER m_contradictionValves)
    Q_PROPERTY (QStringList                 twoOfThree              MEMBER m_twoOfThree) // first twoOfThree case
    Q_PROPERTY (QMap<QString, QStringList>  safetyQuars             MEMBER m_safetyQuars)
public:
    QMap<QString, QStringList>  m_contradictionValves;
    QStringList                 m_twoOfThree;
    QMap<QString, QStringList>  m_safetyQuars;
};

class Initialize : public QObject
{
    Q_OBJECT
public:
    Initialize(QObject *parent = 0, const QString &curInitProfile = "GRAM50");
    Q_PROPERTY(QVariantMap profileJson MEMBER m_profileJson CONSTANT)
    Q_PROPERTY(QStringList profileNames MEMBER m_profileNames CONSTANT)
    Q_PROPERTY(QStringList advantechDeviceMap MEMBER m_advantechDeviceMap NOTIFY advantechDeviceMapChanged)
    Q_PROPERTY(hardwareParameters hardware MEMBER m_hardware CONSTANT)
    Q_PROPERTY(addRemoveQuarParameters addRemoveQuar MEMBER m_addRemoveQuar CONSTANT)
    Q_PROPERTY(storageQuarParameters storageQuar MEMBER m_storageQuar CONSTANT)
    Q_PROPERTY(reactionQuarParameters reactionQuar MEMBER m_reactionQuar CONSTANT)
    Q_PROPERTY(secondLineQuarParameters secondLineQuar MEMBER m_secondLineQuar CONSTANT)
    Q_PROPERTY(securityParameters security MEMBER m_security CONSTANT)

    Q_PROPERTY(QList<daqParameters> daqGui MEMBER m_daq CONSTANT ) // profiled but changing state should be external
    
    void getParametersDO(daqParameters &params); //
    void getParametersAIpres(daqParameters &params); //
    void getParametersAItemp(daqParameters &params); //
    QList<PressureSensor> getPressureSensors() const;
    QStringList getTempSensors() const;


    hardwareParameters              m_hardware; // need m_valves and m_twoOfThree
    securityParameters              m_security; // need m_contradictionValves
    addRemoveQuarParameters         m_addRemoveQuar; // need m_gasSupplyValves
    reactionQuarParameters          m_reactionQuar; // need m_gasLeakageValves and to ValveToRangePressure
    storageQuarParameters           m_storageQuar; // need to ValveToRangePressure

    bool isInitializeOk() const;
signals:
    void advantechDeviceMapChanged();
    //void advantechDeviceSettingsChanged();
    void daqGuiChanged();
    
private:
    bool readProfile(QString &rawData);
    bool jsonParser(QString &rawData, QJsonObject &profileJson);
    bool advantechDeviceCheck();
    void advantechCompareProfile();

    QString m_curInitProfile;
    
    QVariantMap m_profileJson;

    QStringList m_profileNames;

    QStringList m_advantechDeviceMap;

    //QJsonObject profileJson;
    //QVariantMap m_advantechDeviceSettings;

    void visualRepresentation(const QJsonObject &profileJson);
    void fillAddRemoveQuar(const QJsonObject &addRemoveQuarObject);
    void fillStorageQuar(const QJsonObject &storageQuarObject);
    void fillReactionQuar(const QJsonObject &reactionQuarObject);
    void fillSecondLineQuar(const QJsonObject &secondLineQuarObject);

    
    QList<daqParameters>            m_daq;
    secondLineQuarParameters        m_secondLineQuar;
    bool initializeOk;
};