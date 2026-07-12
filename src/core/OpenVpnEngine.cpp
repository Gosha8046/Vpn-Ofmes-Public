#include "vpnofmes/core/OpenVpnEngine.h"
#include "vpnofmes/core/Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTimer>

namespace vpnofmes::core {

OpenVpnEngine::OpenVpnEngine(QObject* parent) : VpnEngine(parent) {}

QString OpenVpnEngine::locateExecutable() const {
    const QStringList candidates = {
        QStringLiteral("C:/Program Files/OpenVPN/bin/openvpn.exe"),
        QStringLiteral("C:/Program Files (x86)/OpenVPN/bin/openvpn.exe"),
        QStandardPaths::findExecutable(QStringLiteral("openvpn.exe")),
        QStandardPaths::findExecutable(QStringLiteral("openvpn"))
    };
    for (const QString& candidate : candidates) {
        if (!candidate.isEmpty() && QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

void OpenVpnEngine::setState(EngineState newState) {
    m_state = newState;
    emit stateChanged(newState);
}

QString OpenVpnEngine::writeAuthFile(const QString& login, const QString& password) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/auth";
    QDir().mkpath(dir);
    const QString path = QDir(dir).filePath("ovpn_auth.txt");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    // OpenVPN's --auth-user-pass file format: username on line 1, password on line 2.
    file.write(login.toUtf8());
    file.write("\n");
    file.write(password.toUtf8());
    file.write("\n");
    file.close();

#ifdef Q_OS_WIN
    // Restrict the file to the current user only; best-effort on non-Windows dev builds.
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
#endif
    return path;
}

void OpenVpnEngine::cleanupAuthFile() {
    if (!m_authFilePath.isEmpty()) {
        QFile::remove(m_authFilePath);
        m_authFilePath.clear();
    }
}

void OpenVpnEngine::connectToServer(const ServerProfile& profile,
                                    const QString& login,
                                    const QString& password) {
    const QString exe = locateExecutable();
    if (exe.isEmpty()) {
        emit connectionFailed(QStringLiteral("OpenVPN executable not found. Install it from openvpn.net."));
        return;
    }

    if (profile.configTemplatePath().isEmpty() || !QFile::exists(profile.configTemplatePath())) {
        emit connectionFailed(QStringLiteral("No OpenVPN config template configured for '%1'").arg(profile.name()));
        return;
    }

    m_userRequestedDisconnect = false;
    setState(EngineState::Starting);

    // Ask the OS for a free local TCP port for the management interface
    // instead of hard-coding one, to avoid clashing with another instance.
    {
        QTcpServer probe;
        probe.listen(QHostAddress::LocalHost, 0);
        m_managementPort = probe.serverPort();
        probe.close();
    }

    QStringList args = {
        "--config", QDir::toNativeSeparators(profile.configTemplatePath()),
        "--management", "127.0.0.1", QString::number(m_managementPort),
        "--management-query-passwords",
        "--auth-nocache",
        "--verb", "3"
    };

    if (!login.isEmpty()) {
        m_authFilePath = writeAuthFile(login, password);
        if (!m_authFilePath.isEmpty()) {
            args << "--auth-user-pass" << QDir::toNativeSeparators(m_authFilePath);
        }
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
            emit logMessage(QStringLiteral("[openvpn] %1").arg(line.trimmed()));
        }
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString output = QString::fromLocal8Bit(m_process->readAllStandardError());
        for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
            emit logMessage(QStringLiteral("[openvpn:stderr] %1").arg(line.trimmed()));
        }
    });
    connect(m_process, &QProcess::finished, this, &OpenVpnEngine::onProcessFinished);

    m_managementSocket = new QTcpSocket(this);
    connect(m_managementSocket, &QTcpSocket::connected, this, &OpenVpnEngine::onManagementConnected);
    connect(m_managementSocket, &QTcpSocket::readyRead, this, &OpenVpnEngine::onManagementReadyRead);

    emit logMessage(QStringLiteral("Starting OpenVPN for '%1'...").arg(profile.name()));
    m_process->start(exe, args);

    // The management listener needs a brief moment to open after the
    // process starts; retry the connection a few times.
    auto attemptsRemaining = std::make_shared<int>(20);
    auto retryTimer = std::make_shared<QTimer>();
    retryTimer->setInterval(250);
    connect(retryTimer.get(), &QTimer::timeout, this,
            [this, attemptsRemaining, retryTimer]() {
                if (m_managementSocket->state() == QAbstractSocket::ConnectedState) {
                    retryTimer->stop();
                    return;
                }
                if (--(*attemptsRemaining) <= 0) {
                    retryTimer->stop();
                    return;
                }
                m_managementSocket->connectToHost(QHostAddress::LocalHost, m_managementPort);
            });
    retryTimer->start();
}

void OpenVpnEngine::onManagementConnected() {
    emit logMessage(QStringLiteral("Connected to OpenVPN management interface"));
    // Ask the daemon to keep pushing state-change notifications.
    m_managementSocket->write("state on\r\n");
}

void OpenVpnEngine::onManagementReadyRead() {
    const QString data = QString::fromLatin1(m_managementSocket->readAll());
    for (const QString& rawLine : data.split('\n', Qt::SkipEmptyParts)) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            continue;
        }

        if (line.startsWith(">HOLD:")) {
            // The daemon pauses on start-up until told to proceed.
            m_managementSocket->write("hold release\r\n");
            continue;
        }

        if (line.startsWith(">STATE:")) {
            emit logMessage(QStringLiteral("[openvpn:state] %1").arg(line));
            if (line.contains(QStringLiteral(",CONNECTED,SUCCESS"))) {
                setState(EngineState::Connected);
                emit connected();
            } else if (line.contains(QStringLiteral(",AUTH_FAILED"))) {
                setState(EngineState::Failed);
                emit connectionFailed(QStringLiteral("OpenVPN authentication failed"));
            }
            continue;
        }

        if (line.contains(QStringLiteral("AUTH_FAILED"))) {
            setState(EngineState::Failed);
            emit connectionFailed(QStringLiteral("OpenVPN authentication failed"));
        }
    }
}

void OpenVpnEngine::disconnectFromServer() {
    if (!m_process || m_process->state() == QProcess::NotRunning) {
        setState(EngineState::Idle);
        emit disconnected();
        return;
    }

    m_userRequestedDisconnect = true;
    setState(EngineState::Stopping);

    if (m_managementSocket && m_managementSocket->state() == QAbstractSocket::ConnectedState) {
        emit logMessage(QStringLiteral("Requesting graceful OpenVPN shutdown..."));
        m_managementSocket->write("signal SIGTERM\r\n");
        m_managementSocket->flush();
    } else {
        m_process->terminate();
    }

    // Fall back to a hard kill if the process does not exit promptly.
    QTimer::singleShot(5000, this, [this]() {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            emit logMessage(QStringLiteral("OpenVPN did not exit gracefully, killing process"));
            m_process->kill();
        }
    });
}

void OpenVpnEngine::onProcessFinished(int exitCode, QProcess::ExitStatus /*status*/) {
    cleanupAuthFile();

    if (m_managementSocket) {
        m_managementSocket->disconnectFromHost();
        m_managementSocket->deleteLater();
        m_managementSocket = nullptr;
    }

    if (m_userRequestedDisconnect || exitCode == 0) {
        setState(EngineState::Idle);
        emit logMessage(QStringLiteral("OpenVPN process stopped"));
        emit disconnected();
    } else {
        setState(EngineState::Failed);
        emit connectionFailed(QStringLiteral("OpenVPN process exited unexpectedly (code %1)").arg(exitCode));
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

} // namespace vpnofmes::core
