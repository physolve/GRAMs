#pragma once

// Пользовательские события машины демо-данных. Все команды (RPC, эхо
// клапанов, такт опроса) доходят до QStateMachine только через postEvent.

#include <QEvent>
#include <QString>

namespace sim {

namespace detail {
inline QEvent::Type registerType()
{
    return static_cast<QEvent::Type>(QEvent::registerEventType());
}
} // namespace detail

// Шаблон «событие с уникальным типом»: тип регистрируется один раз на класс.
template <typename Derived>
class SimEvent : public QEvent {
public:
    SimEvent() : QEvent(staticType()) {}
    static QEvent::Type staticType()
    {
        static const QEvent::Type t = detail::registerType();
        return t;
    }
};

// Такт опроса DataAcquisition: переходы timeout/valve проверяются здесь,
// в порядке перечисления в профиле.
class SimTickEvent final : public SimEvent<SimTickEvent> {};

// Клапан перешёл в состояние (фронт). Порождается SimValveEcho.
class SimValveEvent final : public SimEvent<SimValveEvent> {
public:
    SimValveEvent(const QString &valveId, bool open) : valveId(valveId), open(open) {}
    QString valveId;
    bool open;
};

// Ручной переход sim.goto.
class SimGotoEvent final : public SimEvent<SimGotoEvent> {
public:
    explicit SimGotoEvent(const QString &phaseId) : phaseId(phaseId) {}
    QString phaseId;
};

class SimPauseEvent final : public SimEvent<SimPauseEvent> {};
class SimResumeEvent final : public SimEvent<SimResumeEvent> {};
class SimStopEvent final : public SimEvent<SimStopEvent> {};

class SimFaultEvent final : public SimEvent<SimFaultEvent> {
public:
    explicit SimFaultEvent(const QString &reason) : reason(reason) {}
    QString reason;
};

// Запуск прогона: машина фаз строится из профиля в обработчике события.
class SimRunEvent final : public SimEvent<SimRunEvent> {};

} // namespace sim
