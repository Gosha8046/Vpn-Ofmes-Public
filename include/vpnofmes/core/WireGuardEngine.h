#pragma once

#include <QProcess>

#include "vpnofmes/core/VpnEngine.h"

namespace vpnofmes::core {

/// Drives the official "WireGuard for Windows" client (wireguard.exe) as a
/// tunnel service, exactly the way the WireGuard project documents for
/// headless/scripted control:
///
///   wireguard.exe /installtunnelservice <path-to-tunnel.conf>
///   wireguard.exe /uninstalltunnelservice <TunnelName>
///
/// The tunnel name WireGuard uses is the .conf file's base name, so the
/// profile's config template is copied to a working file named after the
/// profile before installation. All key material and cryptography stay
/// entirely inside the WireGuard driver/service; this class never touches
/// key bytes, it only shells out to the vendor binary.
class WireGuardEngine : public VpnEngine {
    Q_OBJECT

public:
    explicit WireGuardEngine(QObject* parent = nullptr);

    [[nodiscard]] QString engineName() const override { return QStringLiteral("WireGuard"); }
    [[nodiscard]] QString locateExecutable() const override;
    [[nodiscard]] EngineState state() const override { return m_state; }

    void connectToServer(const ServerProfile& profile,
                         const QString& login,
                         const QString& password) override;
    void disconnectFromServer() override;

private:
    void setState(EngineState newState);
    QString workingConfigPath(const ServerProfile& profile) const;

    EngineState m_state = EngineState::Idle;
    QString m_activeTunnelName;
};

} // namespace vpnofmes::core
