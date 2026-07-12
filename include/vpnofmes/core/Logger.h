#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>

namespace vpnofmes::core {

/// Severity levels used when writing an entry to the event log.
enum class LogLevel {
    Info,
    Warning,
    Error,
    Debug
};

/// Application-wide event logger.
///
/// Writes timestamped entries to a rolling log file under logs/ and
/// broadcasts every entry via the entryAdded() signal so the UI (LogsWidget)
/// can display it in real time. Implemented as a singleton because every
/// module in the application (VPN engines, network monitor, update checker,
/// UI) needs a single shared sink.
class Logger : public QObject {
    Q_OBJECT

public:
    static Logger& instance();

    /// Opens (or creates) the log file under the given directory.
    /// Must be called once during application start-up.
    void init(const QString& logDirectory);

    void log(LogLevel level, const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void debug(const QString& message);

    /// Returns every entry written since the logger was initialized,
    /// formatted the same way as entryAdded().
    [[nodiscard]] QStringList history() const;

    /// Human readable path to the active log file.
    [[nodiscard]] QString logFilePath() const;

signals:
    /// Emitted for every log entry, already formatted for display.
    void entryAdded(const QString& formattedLine, LogLevel level);

private:
    Logger() = default;
    ~Logger() override;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static QString levelToString(LogLevel level);

    mutable QMutex m_mutex;
    QFile m_file;
    QString m_filePath;
    QStringList m_history;
};

} // namespace vpnofmes::core
