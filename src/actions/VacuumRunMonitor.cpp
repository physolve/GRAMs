#include "VacuumRunMonitor.h"

// ═════════════════════════════════════════════════════════════════════════════
// VacuumStepModel — статичная развёртка узлов рецепта
// ═════════════════════════════════════════════════════════════════════════════

namespace {

// Порядок строго = порядок VacuumNode (обход дерева сверху вниз).
struct NodeMeta {
    VacuumNode node;
    const char* label;
    const char* phase;
    const char* sRange;
    int depth;
};

const NodeMeta kNodes[] = {
    { VacuumNode::Condition,     "condition-фаза",         "Условие", "",        0 },
    { VacuumNode::F1Relief,      "сброс-триплет К118",     "Ф1",      "s1–s4",   0 },
    { VacuumNode::F2BlockC,      "блок C",                 "Ф2",      "s5–s19",  0 },
    { VacuumNode::F2_RK300,      "подключение C1 / RK300 (S3)", "",   "s5–s10",  1 },
    { VacuumNode::F2_RK10,       "подключение C3 / RK10 (S1)",  "",   "s11–s12", 1 },
    { VacuumNode::F2_RK50,       "подключение C2 / RK50 (S2)",  "",   "s13–s14", 1 },
    { VacuumNode::F2_ReliefMid,  "средний сброс-триплет",  "",        "s15–s17", 1 },
    { VacuumNode::F3SecondTract, "второй тракт",           "Ф3",      "s20–s24", 0 },
    { VacuumNode::F5A1,          "форвакуум A1",           "11.5",    "К118→К178→К176", 0 },
    { VacuumNode::F6BC,          "форвакуум B/C",          "11.6",    "C→К178→К176",    0 },
    { VacuumNode::F7EF,          "форвакуум E/F",          "11.7",    "К151→К178→К176", 0 },
};

} // namespace

VacuumStepModel::VacuumStepModel(QObject* parent)
    : QAbstractListModel(parent)
{
    for (const NodeMeta& m : kNodes)
        m_rows.push_back({ m.node, QString::fromUtf8(m.label),
                           QString::fromUtf8(m.phase), QString::fromUtf8(m.sRange),
                           m.depth, NodeState::Initial });
}

int VacuumStepModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant VacuumStepModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_rows.size()))
        return {};
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case NodeIdRole: return int(r.node);
    case LabelRole:  return r.label;
    case PhaseRole:  return r.phase;
    case SRangeRole: return r.sRange;
    case DepthRole:  return r.depth;
    case StateRole:  return int(r.state);
    default:         return {};
    }
}

QHash<int, QByteArray> VacuumStepModel::roleNames() const
{
    return {
        { NodeIdRole, "nodeId" },
        { LabelRole,  "label"  },
        { PhaseRole,  "phase"  },
        { SRangeRole, "sRange" },
        { DepthRole,  "depth"  },
        { StateRole,  "state"  },
    };
}

int VacuumStepModel::indexOf(VacuumNode node) const
{
    for (int i = 0; i < int(m_rows.size()); ++i)
        if (m_rows.at(i).node == node)
            return i;
    return -1;
}

void VacuumStepModel::resetStates()
{
    for (Row& r : m_rows)
        r.state = NodeState::Initial;
    if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(int(m_rows.size()) - 1), { StateRole });
}

void VacuumStepModel::setNodeState(VacuumNode node, NodeState state)
{
    const int row = indexOf(node);
    if (row < 0)
        return;
    m_rows[row].state = state;
    emit dataChanged(index(row), index(row), { StateRole });
}

// ═════════════════════════════════════════════════════════════════════════════
// VacuumRunMonitor
// ═════════════════════════════════════════════════════════════════════════════

VacuumRunMonitor::VacuumRunMonitor(QObject* parent)
    : QObject(parent)
    , m_steps(new VacuumStepModel(this))
{
    initValves();
}

QAbstractItemModel* VacuumRunMonitor::steps() const
{
    return m_steps;
}

void VacuumRunMonitor::initValves()
{
    // Метки DO-каналов GRAMs (соответствие легаси-клапанам в VacuumTaskTree.h).
    m_valveStates = QVariantMap {
        { VacuumValve::K118, false },  // AR4
        { VacuumValve::K135, false },  // S3  (C1)
        { VacuumValve::K131, false },  // S1  (C3)
        { VacuumValve::K133, false },  // S2  (C2)
        { VacuumValve::K192, false },  // SL2
        { VacuumValve::K179, false },  // SL1
        { VacuumValve::K176, false },  // AR6 — форвакуумный насос
        { VacuumValve::K178, false },  // AR5 — магистраль
        { VacuumValve::K151, false },  // R3  — линия E/F
    };
}

void VacuumRunMonitor::beginRun(int totalRepeats)
{
    m_steps->resetStates();
    initValves();
    m_totalRepeats  = qMax(1, totalRepeats);
    m_currentRepeat = 0;
    m_elapsedSec    = 0;
    m_progress      = 0;
    m_repeatsDone   = 0;
    m_repeatsError  = 0;
    m_currentLabel.clear();
    emit valveStatesChanged();
    emit progressChanged();
    emit currentLabelChanged();
}

void VacuumRunMonitor::reset()
{
    setRunning(false);
    setPaused(false);
    beginRun(m_totalRepeats);
}

void VacuumRunMonitor::onNode(VacuumNode node, NodeState state)
{
    m_steps->setNodeState(node, state);
}

void VacuumRunMonitor::onLabel(const QString& label)
{
    if (m_currentLabel == label)
        return;
    m_currentLabel = label;
    emit currentLabelChanged();
}

void VacuumRunMonitor::onValve(const QString& name, bool open)
{
    if (m_valveStates.value(name).toBool() == open && m_valveStates.contains(name))
        return;
    m_valveStates.insert(name, open);
    emit valveStatesChanged();
}

void VacuumRunMonitor::onProgress(int elapsedSec, int repeat)
{
    m_elapsedSec    = elapsedSec;
    m_currentRepeat = repeat;
    emit progressChanged();
}

void VacuumRunMonitor::onConditionProgress(int elapsedSec, int repeat)
{
    // Условие показываем тем же полем elapsed (отдельная фаза отражена узлом).
    m_elapsedSec    = elapsedSec;
    m_currentRepeat = repeat;
    emit progressChanged();
}

void VacuumRunMonitor::onRepeatDone(bool success, int repeat)
{
    Q_UNUSED(repeat)
    if (success)
        ++m_repeatsDone;
    else
        ++m_repeatsError;
    emit progressChanged();
}

void VacuumRunMonitor::onRunFinished(int repeatsDone, int repeatsError)
{
    m_repeatsDone  = repeatsDone;
    m_repeatsError = repeatsError;
    setRunning(false);
    emit progressChanged();
}

void VacuumRunMonitor::setProgress(int value)
{
    if (m_progress == value)
        return;
    m_progress = value;
    emit progressChanged();
}

void VacuumRunMonitor::setProgressMax(int max)
{
    if (m_progressMax == max)
        return;
    m_progressMax = max;
    emit progressChanged();
}

void VacuumRunMonitor::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged();
}

void VacuumRunMonitor::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    emit pausedChanged();
}
