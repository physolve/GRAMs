#include "RegimeLogger.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QDir>
#include <QDebug>

static const QString kConnectionName = "regime_log_db";

// ─────────────────────────────────────────────────────────────────────────────

RegimeLogger::RegimeLogger(QObject* parent)
    : QObject(parent)
{}

RegimeLogger::~RegimeLogger()
{
    if (m_db.isOpen())
        m_db.close();
    QSqlDatabase::removeDatabase(kConnectionName);
}

bool RegimeLogger::init(const QString& dbPath)
{
    // Ensure data/ directory exists
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    m_db = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "RegimeLogger: cannot open" << dbPath
                   << m_db.lastError().text();
        return false;
    }

    if (!createSchema()) {
        m_db.close();
        return false;
    }

    m_open = true;
    qDebug() << "RegimeLogger: opened" << dbPath;
    return true;
}

bool RegimeLogger::isOpen() const
{
    return m_open;
}

// ── Schema ────────────────────────────────────────────────────────────────────

bool RegimeLogger::createSchema()
{
    QSqlQuery q(m_db);

    // ── regime_runs ──────────────────────────────────────────────────────────
    // One row per execution of one regime (all repeats together).
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS regime_runs (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            regime_index  INTEGER  NOT NULL,
            regime_name   TEXT     NOT NULL,
            started_at    TEXT     NOT NULL,
            finished_at   TEXT,
            status        TEXT     NOT NULL DEFAULT 'running',
            total_repeats INTEGER  NOT NULL DEFAULT 1,
            repeats_done  INTEGER  NOT NULL DEFAULT 0,
            repeats_error INTEGER  NOT NULL DEFAULT 0
        )
    )")) {
        qWarning() << "RegimeLogger: create regime_runs failed:" << q.lastError().text();
        return false;
    }

    // ── regime_events ────────────────────────────────────────────────────────
    // Granular event timeline within a run.
    // detail: free text or JSON for extra structured info
    // (e.g. valve name, pressure value, security map).
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS regime_events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id      INTEGER NOT NULL REFERENCES regime_runs(id) ON DELETE CASCADE,
            ts          TEXT    NOT NULL,
            event_type  TEXT    NOT NULL,
            repeat_num  INTEGER DEFAULT -1,
            elapsed_sec INTEGER DEFAULT -1,
            detail      TEXT
        )
    )")) {
        qWarning() << "RegimeLogger: create regime_events failed:" << q.lastError().text();
        return false;
    }

    // Indices for fast queries by run and time
    q.exec("CREATE INDEX IF NOT EXISTS idx_events_run ON regime_events(run_id)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_events_ts  ON regime_events(ts)");

    return true;
}

// ── Run lifecycle ─────────────────────────────────────────────────────────────

qint64 RegimeLogger::openRun(int regimeIndex, const QString& regimeName, int totalRepeats)
{
    if (!m_open) return -1;

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO regime_runs (regime_index, regime_name, started_at, total_repeats)
        VALUES (?, ?, ?, ?)
    )");
    q.addBindValue(regimeIndex);
    q.addBindValue(regimeName);
    q.addBindValue(nowIso());
    q.addBindValue(totalRepeats);

    if (!q.exec()) {
        qWarning() << "RegimeLogger::openRun failed:" << q.lastError().text();
        return -1;
    }

    qint64 runId = q.lastInsertId().toLongLong();
    logEvent(runId, kRegimeStart, -1, -1, regimeName);
    return runId;
}

void RegimeLogger::closeRun(qint64 runId, const QString& status,
                             int repeatsDone, int repeatsError)
{
    if (!m_open || runId < 0) return;

    QSqlQuery q(m_db);
    q.prepare(R"(
        UPDATE regime_runs
        SET finished_at   = ?,
            status        = ?,
            repeats_done  = ?,
            repeats_error = ?
        WHERE id = ?
    )");
    q.addBindValue(nowIso());
    q.addBindValue(status);
    q.addBindValue(repeatsDone);
    q.addBindValue(repeatsError);
    q.addBindValue(runId);

    if (!q.exec())
        qWarning() << "RegimeLogger::closeRun failed:" << q.lastError().text();
}

// ── Event log ─────────────────────────────────────────────────────────────────

void RegimeLogger::logEvent(qint64 runId, const QString& eventType,
                             int repeatNum, int elapsedSec, const QString& detail)
{
    if (!m_open || runId < 0) return;

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO regime_events (run_id, ts, event_type, repeat_num, elapsed_sec, detail)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(runId);
    q.addBindValue(nowIso());
    q.addBindValue(eventType);
    q.addBindValue(repeatNum < 0 ? QVariant(QMetaType(QMetaType::Int)) : QVariant(repeatNum));
    q.addBindValue(elapsedSec < 0 ? QVariant(QMetaType(QMetaType::Int)) : QVariant(elapsedSec));
    q.addBindValue(detail.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : QVariant(detail));

    if (!q.exec())
        qWarning() << "RegimeLogger::logEvent failed:" << q.lastError().text();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

QString RegimeLogger::nowIso() const
{
    return QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
}
