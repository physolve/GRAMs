#pragma once

#include "controllers/AdvantechCtrl.h"
#include "controllers/IDoPort.h"
#include "controllers/RealDoPort.h"
#include "Initialize.h"

#include <memory>
#include <QLoggingCategory>

// grams.valves — путь команды клапана: запрос → Security → порт → GUI
// (QT_LOGGING_RULES="grams.valves.debug=true").
Q_DECLARE_LOGGING_CATEGORY(lcValves)
#include "addon/AddRemoveQuartile.h"
#include "addon/ReactionQuartile.h"
#include "db/GramStateDB.h"
#include "DataCollection.h"
#include "ValveModel.h" // Vlave object
#include "Security.h"

class ValveControl : public QObject
{
    Q_OBJECT
    Q_PROPERTY (QVariantMap guiValve READ getGuiValsValve NOTIFY guiValsValveChanged)
public:
    ValveControl(QObject *parent = 0);
    ~ValveControl();
    void setValvePointers(const QVector<Valve*>& ptr);
    void setChamberValvePointer(Valve* chamberValve);
    void setAddRemoveQuartile(AddRemoveQuartile* addRemoveQuartile);
    // void setStorageQuartile(StorageQuartile* storageQuartile);
    void setReactionQuartile(ReactionQuartile* reactionQuartile);
    void setDatabase(GramStateDB* gramStateDB);
    void setSafeModule(Security* safeModule);
    void setGasSupplyValves(const QStringList& gasSupplyValves);
    void setGasStoreValves(const QStringList& gasStoreValves);
    void initDaqDO(const daqParameters &parameterDO);
    // Порт выходов клапанов. initDaqDO ставит плату USB-4750; демо-режим —
    // эхо записи (sim::SimValveEcho). Security и логика клапанов те же.
    void setDoPort(std::unique_ptr<IDoPort> port);
    void beginAction();
    void endAction();
    bool isActionInterrupted() const;
    bool setValveFromAction(bool state, const QString& name);
    // Подтверждение ФАКТА состояния клапана (V-02, REQ-082/084).
    //
    // setValveFromAction сообщает результат КОМАНДЫ: «Security не запретил и
    // Write вернул Success». Этого мало для перехода на турбомолекулярный
    // насос — открыть К179 на непрощавшемся форвакуумном тракте значит
    // испортить насос. confirmValve перечитывает состояние с платы.
    //
    // Ограничение: чтение DO — это регистр-защёлка выхода, не датчик положения
    // (подробности в AdvantechDO::refresh). Без сконфигурированной платы
    // возвращает false: недоказанное состояние считаем неподтверждённым.
    bool confirmValve(const QString& name, bool expected);
    // Кэш последней команды (без обращения к железу). known = false, если
    // имени нет в реестре.
    bool valveState(const QString& name, bool* known = nullptr) const;
    // Состояния всех клапанов реестра (без R5) — вход проверок Security.
    QMap<QString, bool> valveStates() const;
    // Кто держит клапан открытым (см. ValveSource) — снимок для Security.
    ValveSource valveSource(const QString& name) const;
    QMap<QString, ValveSource> valveSources() const;
    // Код причины, по которой Security отклонил последнюю команду (interlock,
    // pressure_range, pressure_invalid); пусто, если команда разрешена.
    QString lastRefusal() const { return m_lastRefusal; }
    // Правила Security, которые действуют без команды: клапаны диапазона
    // (S4/R4), которые Security требует закрыть, закрываются здесь — той же
    // записью в порт, что и обычная команда, поэтому мнемосхема, Lumber, БД и
    // демо видят одно и то же. Вызывается на каждом такте softEvent (origin =
    // "tick"), при старте и из readback. Security клапаны только закрывает.
    void enforceSecurity(const QString& origin);
    // Идёт ли режим (RegimeTaskTree: regimeStarted / regimeFinished). Клапан
    // диапазона, открытый режимом, держится открытым только пока режим идёт;
    // конец режима (успех, стоп, ошибка) закрывает его — origin regime_end.
    void setRegimeActive(bool active);
    bool isRegimeActive() const { return m_regimeActive; }
    Q_INVOKABLE void setManualChamberValve(bool state);
    Q_INVOKABLE void setValveState(bool state, int valveId);
    bool sendValveStates();
    bool isControlRunning();
    void valveChangeUpdater(const QString& valveName, bool newState);
    QVariantMap getGuiValsValve() const;

signals:
    void guiValsValveChanged();
    // Security закрыл клапан; previous — кто держал его открытым. Режим,
    // открывший клапан (previous == Regime), обязан узнать об этом (Т3).
    void securityClosed(const SecurityIssue& issue, ValveSource previous);
    // Недостоверное показание у открытого клапана — один раз до восстановления.
    void securityWarning(const SecurityIssue& issue);

private:
    std::unique_ptr<IDoPort> m_doPort;   // nullptr, пока порт не поставлен
    bool valveController;
    bool actionInterrupted;
    // valve pointers
    QVector<Valve*> m_valves;
    Valve* m_chamberValve;
    Security* m_safeModule;

    QStringList valveNameList;
    QString m_lastRefusal;
    // Источник открытия по имени клапана; нет записи — None.
    QMap<QString, ValveSource> m_sources;
    // Итог принятой платой команды: открыт — источник команды, закрыт — None.
    void setSource(const QString& name, bool open, ValveSource commandSource);
    // Закрыть клапан по требованию Security; false — плата не приняла запись.
    bool closeBySecurity(const SecurityIssue& issue);
    // Перед открытием перепускного клапана (К151/К153/К155): закрыть клапаны
    // диапазона, которым прогноз равновесия выше порога, — отдельной записью
    // до открытия. false — закрыть не удалось, открывать нельзя.
    bool prepareTransferOpen(const QString& name);
    // Клапаны, закрыть которые не удалось: о повторных неудачах не сообщаем.
    QSet<QString> m_securityCloseFailed;
    bool m_regimeActive = false;
    QStringList m_gasSupplyValves;
    QStringList m_gasStoreValves;
    // add remove pointer
    AddRemoveQuartile* m_addRemoveQuartile;
    // storage pointer
    // reaction pointer
    ReactionQuartile* m_reactionQuartile;
    // database pointer
    GramStateDB* m_gramStateDB;
    bool checkOpenChamber(bool state);
};