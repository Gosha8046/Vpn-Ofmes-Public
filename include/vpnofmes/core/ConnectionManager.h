#pragma once

#include <QObject>
#include <QTimer>
#include <memory>

#include "vpnofmes/core/ConfigManager.h"
#include "vpnofmes/core/ServerProfile.h"
#include "vpnofmes/core/VpnEngine.h"

namespace vpnofmes::core {

/// High-level connection state shown by the UI. Distinct from EngineState:
/// this adds the Reconnecting state that only ConnectionManager knows
/// about (it lives above any single engine instance).
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Error
};

/// Orchestrates whichever VpnEngine matches the selected server's
/// protocol, and layers auto-reconnect behaviour on top of it.
///
/// This class contains no VPN protocol logic itself -- it only decides
/// *when* to call connectToServer()/disconnectFromServer() on the engine
/// and translates engine-level events into the application-level
/// ConnectionState the UI cares about.
class ConnectionManager : public QObject {
    Q_OBJECT

public:
    explicit ConnectionManager(ConfigManager& configManager, QObject* parent = nullptr);

    void connectTo(const ServerProfile& profile, const QString& login, const QString& password);
    void disconnectFromVpn();

    [[nodiscard]] ConnectionState state() const { return m_state; }
    [[nodiscard]] QString activeEngineName() const;

signals:
    void stateChanged(vpnofmes::core::ConnectionState state);
    void logMessage(const QString& message);

private:
    VpnEngine* engineFor(VpnProtocol protocol);
    void setState(ConnectionState newState);
    void scheduleReconnect();
    void resetReconnectAttempts();

    ConfigManager& m_configManager;
    std::unique_ptr<VpnEngine> m_wireGuardEngine;
    std::unique_ptr<VpnEngine> m_openVpnEngine;
    VpnEngine* m_activeEngine = nullptr;

    ServerProfile m_currentProfile;
    QString m_currentLogin;
    QString m_currentPassword;

    ConnectionState m_state = ConnectionState::Disconnected;
    bool m_userInitiatedDisconnect = false;
    int m_reconnectAttempts = 0;
    QTimer m_reconnectTimer;
};

} // namespace vpnofmes::core
