#include "vpnofmes/core/Logger.h"

#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

namespace vpnofmes::core {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (m_file.isOpen()) {
        m_file.close();
    }
}

void Logger::init(const QString& logDirectory) {
    QMutexLocker locker(&m_mutex);

    QDir dir(logDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    const QString fileName = QStringLiteral("vpnofmes_%1.log")
                                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    m_filePath = dir.filePath(fileName);

    if (m_file.isOpen()) {
        m_file.close();
    }
    m_file.setFileName(m_filePath);
    m_file.open(QIODevice::Append | QIODevice::Text);
}

QString Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return QStringLiteral("INFO");
        case LogLevel::Warning: return QStringLiteral("WARN");
        case LogLevel::Error:   return QStringLiteral("ERROR");
        case LogLevel::Debug:   return QStringLiteral("DEBUG");
    }
    return QStringLiteral("INFO");
}

void Logger::log(LogLevel level, const QString& message) {
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    const QString line = QStringLiteral("[%1] [%2] %3")
                              .arg(timestamp, levelToString(level), message);

    {
        QMutexLocker locker(&m_mutex);
        m_history.append(line);

        if (m_file.isOpen()) {
            QTextStream stream(&m_file);
            stream << line << Qt::endl;
        }
    }

    emit entryAdded(line, level);
}

void Logger::info(const QString& message) { log(LogLevel::Info, message); }
void Logger::warning(const QString& message) { log(LogLevel::Warning, message); }
void Logger::error(const QString& message) { log(LogLevel::Error, message); }
void Logger::debug(const QString& message) { log(LogLevel::Debug, message); }

QStringList Logger::history() const {
    QMutexLocker locker(&m_mutex);
    return m_history;
}

QString Logger::logFilePath() const {
    QMutexLocker locker(&m_mutex);
    return m_filePath;
}

} // namespace vpnofmes::core
