#include "ChainWorkers.h"

#include <QDebug>

using namespace QtTaskTree;

namespace {

// Общий для обоих воркеров шов клапана: команда + журнал.
std::function<bool(bool, const QString&)> makeSetValve(ValveControl* vc,
                                                       RegimeLogger* logger,
                                                       qint64 runId,
                                                       const char* tag)
{
    return [vc, logger, runId, tag](bool open, const QString& name) -> bool {
        if (!vc) {
            qWarning() << tag << "ValveControl не задан — операция с" << name
                       << "отклонена";
            return false;
        }
        const bool ok = vc->setValveFromAction(open, name);
        if (open && !ok)
            qWarning() << tag << "Открытие клапана" << name << "заблокировано";
        if (logger) {
            const char* type = open ? (ok ? RegimeLogger::kValveOpen
                                          : RegimeLogger::kValveBlocked)
                                    : RegimeLogger::kValveClose;
            logger->logEvent(runId, type, -1, -1, name);
        }
        return ok;
    };
}

std::function<bool(bool, const QString&)> makeConfirmValve(ValveControl* vc, const char* tag)
{
    if (!vc)
        return {};                      // nullable шов: гейт проверит только команду
    return [vc, tag](bool expectedOpen, const QString& name) -> bool {
        const bool ok = vc->confirmValve(name, expectedOpen);
        if (!ok)
            qWarning() << tag << "Readback клапана" << name
                       << "не подтвердил состояние" << expectedOpen;
        return ok;
    };
}

// Показание DataCollection как Reading: значение плюс качество последней точки.
std::function<Reading()> makeReading(DataCollection* sensor)
{
    if (!sensor)
        return {};
    return [sensor] { return Reading(sensor->getCurValue(), sensor->quality()); };
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// SupplyRegimeWorker
// ═════════════════════════════════════════════════════════════════════════════

SupplyRegimeWorker::SupplyRegimeWorker(QObject* parent) : QObject(parent) {}

void SupplyRegimeWorker::setConfig(const RegimeWorkerConfig& cfg) { m_cfg = cfg; }
void SupplyRegimeWorker::setOptions(const SupplyOptions& opts)    { m_opts = opts; }

void SupplyRegimeWorker::onPauseRequested()  { m_pauseBus.pause(); }
void SupplyRegimeWorker::onResumeRequested() { m_pauseBus.resume(); }

SupplyTreeContext SupplyRegimeWorker::makeContext()
{
    SupplyTreeContext ctx;
    ctx.port             = m_opts.port;
    ctx.openTimeMs       = m_opts.openTimeMs;
    ctx.pressureLimitBar = m_opts.pressureLimitBar;
    ctx.gateCheck        = m_opts.gateCheck;
    ctx.gateMaxStartBar  = m_opts.gateMaxStartBar;
    ctx.pauseBus         = &m_pauseBus;

    ctx.setValve     = makeSetValve(m_cfg.valveControl, m_cfg.logger, m_runId, "[Напуск]");
    ctx.confirmValve = makeConfirmValve(m_cfg.valveControl, "[Напуск]");

    if (m_opts.storageSensor) {
        ctx.pressureStorageBar = makeReading(m_opts.storageSensor);
    } else {
        qWarning() << "[Напуск] Датчик накопителя не задан — гейт цепочки "
                      "не сможет проверить, что тракт откачан";
    }

    // beginAction/endAction у ValveControl и DataAcquisition вызываются парой:
    // рассинхронизировать их нечем, поэтому один шов с флагом.
    ValveControl* vc = m_cfg.valveControl;
    DataAcquisition* da = m_cfg.dataAcquisition;
    ctx.setActionMode = [vc, da](bool active) {
        if (active) {
            if (vc) vc->beginAction();
            if (da) da->beginAction();
        } else {
            if (vc) vc->endAction();
            if (da) da->endAction();
        }
    };
    if (da) {
        ctx.fastBufferRead  = [da] { da->fastBufferRead(); };
        ctx.runSupplyAction = [da] { da->runSupplyAction(); };
    }
    if (vc)
        ctx.actionInterrupted = [vc] { return vc->isActionInterrupted(); };

    if (AddRemoveQuartile* q = m_opts.addRemoveQuartile) {
        ctx.fillSupplyData   = [q](int ms) { q->fillSupplyActionData(uint(ms)); };
        ctx.appendSupplyData = [q](int ms) { return q->appendSupplyActionData(uint(ms)); };
        ctx.saveSupplyData   = [q] { q->saveSupplyActionData(); };
        ctx.checkSupplyAction = [q] { return q->checkSupplyAction(); };
    } else {
        qWarning() << "[Напуск] AddRemoveQuartile не задан — точки прогона "
                      "не записываются";
    }

    ctx.onFailure = [this](const QString& reason) {
        qWarning() << "[Напуск] Отказ:" << reason;
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRegimeError, -1, -1, reason);
    };
    ctx.onFinished = [this](SupplyStop stop) {
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatDone, -1, -1,
                                   supplyStopName(stop));
    };
    ctx.onLabel = [](const QString& label) { qDebug() << "[Напуск]" << label; };
    return ctx;
}

void SupplyRegimeWorker::start()
{
    if (m_cfg.logger)
        m_runId = m_cfg.logger->openRun(m_cfg.regimeId, m_cfg.regimeName,
                                        m_cfg.totalRepeats);

    m_tree = new QTaskTree(buildSupplyRecipe(makeContext()), this);
    connect(m_tree, &QTaskTree::done, this, [this](DoneWith w) {
        emit done(w == DoneWith::Success);
    });
    m_tree->start();
}

void SupplyRegimeWorker::cancelTree()
{
    if (!m_tree || !m_tree->isRunning())
        return;
    // Как у VacuumRegimeWorker: рвём done→emit, иначе отменяемое внутреннее
    // дерево реэмитит в уже завершённый QTaskInterface. Клапан закрывают
    // done-хендлеры рецепта во время cancel().
    disconnect(m_tree, &QTaskTree::done, this, nullptr);
    m_tree->cancel();
}

// ═════════════════════════════════════════════════════════════════════════════
// LeakageRegimeWorker
// ═════════════════════════════════════════════════════════════════════════════

LeakageRegimeWorker::LeakageRegimeWorker(QObject* parent) : QObject(parent) {}

void LeakageRegimeWorker::setConfig(const RegimeWorkerConfig& cfg) { m_cfg = cfg; }
void LeakageRegimeWorker::setOptions(const LeakageOptions& opts)   { m_opts = opts; }

void LeakageRegimeWorker::onPauseRequested()  { m_pauseBus.pause(); }
void LeakageRegimeWorker::onResumeRequested() { m_pauseBus.resume(); }

LeakageTreeContext LeakageRegimeWorker::makeContext()
{
    LeakageTreeContext ctx;
    ctx.valve              = m_opts.valve;
    ctx.durationSec        = m_opts.durationSec;
    ctx.targetDeltaBar     = m_opts.targetDeltaBar;
    ctx.gateCheck          = m_opts.gateCheck;
    ctx.gateMinStorageBar  = m_opts.gateMinStorageBar;
    ctx.gateMaxReactionBar = m_opts.gateMaxReactionBar;
    ctx.pauseBus           = &m_pauseBus;

    ctx.setValve     = makeSetValve(m_cfg.valveControl, m_cfg.logger, m_runId, "[Натекание]");
    ctx.confirmValve = makeConfirmValve(m_cfg.valveControl, "[Натекание]");

    ctx.pressureStorageBar  = makeReading(m_opts.storageSensor);
    ctx.pressureReactionBar = makeReading(m_opts.reactionSensor);
    if (DataCollection* t = m_opts.temperatureSensor)
        ctx.temperatureReactionK = [t] { return t->getCurValue(); };
    if (!m_opts.storageSensor || !m_opts.reactionSensor)
        qWarning() << "[Натекание] Датчики давления заданы не полностью — "
                      "гейт цепочки откажет в старте";

    if (DataAcquisition* da = m_cfg.dataAcquisition)
        ctx.setMeasureMode = [da](bool on) { da->setLeakageMeasure(on); };

    // Расчёт натекания живёт в ReactionQuartile/GasLeakage и не переписывается:
    // рецепт только сообщает ему старт, точки и конец.
    if (ReactionQuartile* rq = m_opts.reactionQuartile) {
        ctx.leakageSetOpen = [rq](bool on) { rq->startLeakageMeasure(on); };
    } else {
        qWarning() << "[Натекание] ReactionQuartile не задан — расчёт расхода "
                      "не выполняется, прогон только управляет клапаном";
    }

    ctx.onFailure = [this](const QString& reason) {
        qWarning() << "[Натекание] Отказ:" << reason;
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRegimeError, -1, -1, reason);
    };
    ctx.onFinished = [this](LeakageStop stop) {
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatDone, -1, -1,
                                   leakageStopName(stop));
    };
    ctx.onLabel = [](const QString& label) { qDebug() << "[Натекание]" << label; };
    return ctx;
}

void LeakageRegimeWorker::start()
{
    if (m_cfg.logger)
        m_runId = m_cfg.logger->openRun(m_cfg.regimeId, m_cfg.regimeName,
                                        m_cfg.totalRepeats);

    m_tree = new QTaskTree(buildLeakageRecipe(makeContext()), this);
    connect(m_tree, &QTaskTree::done, this, [this](DoneWith w) {
        emit done(w == DoneWith::Success);
    });
    m_tree->start();
}

void LeakageRegimeWorker::cancelTree()
{
    if (!m_tree || !m_tree->isRunning())
        return;
    disconnect(m_tree, &QTaskTree::done, this, nullptr);
    m_tree->cancel();
}
