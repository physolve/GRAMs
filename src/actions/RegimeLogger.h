#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

// ─────────────────────────────────────────────────────────────────────────────
// RegimeLogger — SQLite-based execution log for all regime runs.
//
// Separate from GramStateDB (PostgreSQL, pressure snapshots).
// File: data/regime_log.db
//
// Schema:
//   regime_runs  — one row per regime execution attempt
//   regime_events — granular event log within a run
//
// Thread-safety: all methods must be called from the main thread.
// ─────────────────────────────────────────────────────────────────────────────

class RegimeLogger : public QObject
{
    Q_OBJECT
public:
    explicit RegimeLogger(QObject* parent = nullptr);
    ~RegimeLogger() override;

    bool init(const QString& dbPath = "data/regime_log.db");
    bool isOpen() const;

    // ── Run lifecycle ─────────────────────────────────────────────────────────

    // Opens a new run record. Returns run_id (>0) on success, -1 on failure.
    qint64 openRun(int regimeIndex, const QString& regimeName, int totalRepeats);

    // Closes an open run record with final status.
    // status: "success" | "error" | "cancelled" | "paused"
    void closeRun(qint64 runId, const QString& status,
                  int repeatsDone, int repeatsError);

    // ── Event log ─────────────────────────────────────────────────────────────
    //
    // event_type values (use constants below):
    //   kRegimeStart, kConditionStart, kConditionDone,
    //   kExecutionStart, kRepeatDone, kRepeatError,
    //   kRegimeDone, kRegimeError, kCancelled,
    //   kPaused, kResumed,
    //   kSecurityViolation,
    //   kValveOpen, kValveClose, kValveBlocked

    void logEvent(qint64 runId,
                  const QString& eventType,
                  int repeatNum   = -1,
                  int elapsedSec  = -1,
                  const QString& detail = {});

    // ── Event type constants ──────────────────────────────────────────────────

    static constexpr const char* kRegimeStart       = "regime_start";
    static constexpr const char* kConditionStart    = "condition_start";
    static constexpr const char* kConditionDone     = "condition_done";
    static constexpr const char* kExecutionStart    = "execution_start";
    static constexpr const char* kRepeatDone        = "repeat_done";
    static constexpr const char* kRepeatError       = "repeat_error";
    static constexpr const char* kRegimeDone        = "regime_done";
    static constexpr const char* kRegimeError       = "regime_error";
    static constexpr const char* kCancelled         = "cancelled";
    static constexpr const char* kPaused            = "paused";
    static constexpr const char* kResumed           = "resumed";
    static constexpr const char* kSecurityViolation = "security_violation";
    static constexpr const char* kValveOpen         = "valve_open";
    static constexpr const char* kValveClose        = "valve_close";
    static constexpr const char* kValveBlocked      = "valve_blocked";

private:
    bool createSchema();
    QString nowIso() const;

    QSqlDatabase m_db;
    bool         m_open = false;
};
