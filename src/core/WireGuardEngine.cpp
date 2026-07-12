#include "vpnofmes/core/WireGuardEngine.h"
#include "vpnofmes/core/Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace vpnofmes::core {

namespace {
/// Sanitizes a profile name into something safe to use as a Windows
/// service/tunnel name and file base name (WireGuard requires the tunnel
/// name to match [a-zA-Z0-9_=+.-]{1,32}).
QString sanitizeTunnelName(const QString& name) {
    QString result;
    for (const QChar& ch : name) {
        if (ch.isLetterOrNumber() || ch == '_' || ch == '-' || ch == '.') {
            result.append(ch);
        } else {
            result.append('_');
        }
    }
    return result.left(32);
}
} // namespace

WireGuardEngine::WireGuardEngine(QObject* parent) : VpnEngine(parent) {}

QString WireGuardEngine::locateExecutable() const {
    const QStringList candidates = {
        QStringLiteral("C:/Program Files/WireGuard/wireguard.exe"),
        QStandardPaths::findExecutable(QStringLiteral("wireguard.exe")),
        QStandardPaths::findExecutable(QStringLiteral("wireguard"))
    };
    for (const QString& candidate : candidates) {
        if (!candidate.isEmpty() && QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString WireGuardEngine::workingConfigPath(const ServerProfile& profile) const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/tunnels";
    QDir().mkpath(dir);
    return QDir(dir).filePath(sanitizeTunnelName(profile.name()) + ".conf");
}

void WireGuardEngine::setState(EngineState newState) {
    m_state = newState;
    emit stateChanged(newState);
}

void WireGuardEngine::connectToServer(const ServerProfile& profile,
                                     const QString& /*login*/,
                                     const QString& /*password*/) {
    // WireGuard authenticates via public/private keypairs baked into the
    // .conf template, not via username/password, so those parameters are
    // intentionally unused here.
    const QString exe = locateExecutable();
    if (exe.isEmpty()) {
        emit connectionFailed(QStringLiteral("WireGuard executable not found. Install it from wireguard.com."));
        return;
    }

    if (profile.configTemplatePath().isEmpty() || !QFile::exists(profile.configTemplatePath())) {
        emit connectionFailed(QStringLiteral("No WireGuard config template configured for '%1'").arg(profile.name()));
        return;
    }

    setState(EngineState::Starting);
    emit logMessage(QStringLiteral("Installing WireGuard tunnel for '%1'...").arg(profile.name()));

    const QString targetPath = workingConfigPath(profile);
    QFile::remove(targetPath);
    if (!QFile::copy(profile.configTemplatePath(), targetPath)) {
        setState(EngineState::Failed);
        emit connectionFailed(QStringLiteral("Could not prepare tunnel configuration file"));
        return;
    }

    m_activeTunnelName = QFileInfo(targetPath).completeBaseName();

    auto* process = new QProcess(this);
    connect(process, &QProcess::finished, this,
            [this, process](int exitCode, QProcess::ExitStatus /*status*/) {
                if (exitCode == 0) {
                    setState(EngineState::Connected);
                    emit logMessage(QStringLiteral("WireGuard tunnel '%1' is up").arg(m_activeTunnelName));
                    emit connected();
                } else {
                    setState(EngineState::Failed);
                    const QString output = QString::fromLocal8Bit(process->readAllStandardError());
                    emit connectionFailed(QStringLiteral("wireguard.exe exited with code %1: %2")
                                              .arg(exitCode).arg(output));
                }
                process->deleteLater();
            });

    process->start(exe, {"/installtunnelservice", QDir::toNativeSeparators(targetPath)});
}

void WireGuardEngine::disconnectFromServer() {
    if (m_activeTunnelName.isEmpty()) {
        setState(EngineState::Idle);
        emit disconnected();
        return;
    }

    const QString exe = locateExecutable();
    if (exe.isEmpty()) {
        setState(EngineState::Idle);
        emit disconnected();
        return;
    }

    setState(EngineState::Stopping);
    emit logMessage(QStringLiteral("Stopping WireGuard tunnel '%1'...").arg(m_activeTunnelName));

    auto* process = new QProcess(this);
    connect(process, &QProcess::finished, this,
            [this, process](int /*exitCode*/, QProcess::ExitStatus /*status*/) {
                setState(EngineState::Idle);
                m_activeTunnelName.clear();
                emit logMessage(QStringLiteral("WireGuard tunnel stopped"));
                emit disconnected();
                process->deleteLater();
            });

    process->start(exe, {"/uninstalltunnelservice", m_activeTunnelName});
}

} // namespace vpnofmes::core
