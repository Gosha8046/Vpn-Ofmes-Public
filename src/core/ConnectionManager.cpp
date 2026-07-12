#include "vpnofmes/core/ConnectionManager.h"
#include "vpnofmes/core/Logger.h"
#include "vpnofmes/core/OpenVpnEngine.h"
#include "vpnofmes/core/WireGuardEngine.h"

namespace vpnofmes::core {

ConnectionManager::ConnectionManager(ConfigManager& configManager, QObject* parent)
    : QObject(parent), m_configManager(configManager) {
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_userInitiatedDisconnect) {
            return;
        }
        ++m_reconnectAttempts;
        emit logMessage(QStringLiteral("Reconnect attempt %1/%2 to '%3'...")
                             .arg(m_reconnectAttempts)
                             .arg(m_configManager.settings().maxReconnectAttempts)
                             .arg(m_currentProfile.name()));
        m_activeEngine->connectToServer(m_currentProfile, m_currentLogin, m_currentPassword);
    });
}

VpnEngine* ConnectionManager::engineFor(VpnProtocol protocol) {
    VpnEngine* engine = nullptr;

    if (protocol == VpnProtocol::WireGuard) {
        if (!m_wireGuardEngine) {
            m_wireGuardEngine = std::make_unique<WireGuardEngine>();
        }
        engine = m_wireGuardEngine.get();
    } else {
        if (!m_openVpnEngine) {
            m_openVpnEngine = std::make_unique<OpenVpnEngine>();
        }
        engine = m_openVpnEngine.get();
    }

    if (engine != m_activeEngine) {
        // Rewire signal/slot connections whenever the active engine changes
        // (e.g. the user switches from a WireGuard server to an OpenVPN one).
        disconnect(engine, nullptr, this, nullptr);

        connect(engine, &VpnEngine::connected, this, [this]() {
            resetReconnectAttempts();
            setState(ConnectionState::Connected);
        });
        connect(engine, &VpnEngine::disconnected, this, [this]() {
            if (m_userInitiatedDisconnect) {
                setState(ConnectionState::Disconnected);
            } else if (m_configManager.settings().autoReconnect &&
                       m_reconnectAttempts < m_configManager.settings().maxReconnectAttempts) {
                scheduleReconnect();
            } else {
                setState(ConnectionState::Disconnected);
            }
        });
        connect(engine, &VpnEngine::connectionFailed, this, [this](const QString& reason) {
            emit logMessage(QStringLiteral("Connection failed: %1").arg(reason));
            if (!m_userInitiatedDisconnect &&
                m_configManager.settings().autoReconnect &&
                m_reconnectAttempts < m_configManager.settings().maxReconnectAttempts) {
                scheduleReconnect();
            } else {
                setState(ConnectionState::Error);
            }
        });
        connect(engine, &VpnEngine::logMessage, this, &ConnectionManager::logMessage);

        m_activeEngine = engine;
    }

    return engine;
}

void ConnectionManager::connectTo(const ServerProfile& profile, const QString& login, const QString& password) {
    m_currentProfile = profile;
    m_currentLogin = login;
    m_currentPassword = password;
    m_userInitiatedDisconnect = false;
    resetReconnectAttempts();

    VpnEngine* engine = engineFor(profile.protocol());
    setState(ConnectionState::Connecting);
    emit logMessage(QStringLiteral("Connecting to '%1' via %2...").arg(profile.name(), engine->engineName()));
    engine->connectToServer(profile, login, password);
}

void ConnectionManager::disconnectFromVpn() {
    m_userInitiatedDisconnect = true;
    m_reconnectTimer.stop();

    if (!m_activeEngine || m_activeEngine->state() == EngineState::Idle) {
        setState(ConnectionState::Disconnected);
        return;
    }

    emit logMessage(QStringLiteral("Disconnecting..."));
    m_activeEngine->disconnectFromServer();
}

void ConnectionManager::scheduleReconnect() {
    setState(ConnectionState::Reconnecting);
    const int delayMs = m_configManager.settings().reconnectIntervalSeconds * 1000;
    emit logMessage(QStringLiteral("Connection lost. Reconnecting in %1s...")
                         .arg(m_configManager.settings().reconnectIntervalSeconds));
    m_reconnectTimer.start(delayMs);
}

void ConnectionManager::resetReconnectAttempts() {
    m_reconnectAttempts = 0;
}

void ConnectionManager::setState(ConnectionState newState) {
    if (m_state == newState) {
        return;
    }
    m_state = newState;
    emit stateChanged(newState);
}

QString ConnectionManager::activeEngineName() const {
    return m_activeEngine ? m_activeEngine->engineName() : QString();
}

} // namespace vpnofmes::core
