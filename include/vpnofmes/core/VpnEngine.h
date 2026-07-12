#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "vpnofmes/core/ServerProfile.h"

namespace vpnofmes::core {

/// Lifecycle state of a VPN engine instance.
enum class EngineState {
    Idle,
    Starting,
    Connected,
    Stopping,
    Failed
};

/// Abstract interface every concrete VPN engine implements.
///
/// Vpn-Ofmes intentionally does NOT implement any tunneling protocol or
/// cryptography of its own. Every subclass is a thin, supervised wrapper
/// around an existing, independently-audited open-source VPN engine
/// (the official WireGuard for Windows tool, or the official OpenVPN
/// binary), driven through the command-line/management interfaces those
/// projects publish for third-party integration. This class only defines
/// the shape that ConnectionManager needs to control any of them
/// uniformly.
class VpnEngine : public QObject {
    Q_OBJECT

public:
    explicit VpnEngine(QObject* parent = nullptr) : QObject(parent) {}
    ~VpnEngine() override;

    /// Human-readable engine name, e.g. "WireGuard" or "OpenVPN".
    [[nodiscard]] virtual QString engineName() const = 0;

    /// Starts connecting to the given server. Credentials are only used by
    /// engines that support username/password authentication (OpenVPN);
    /// key-based engines (WireGuard) ignore them since the keys already
    /// live in the profile's config template.
    virtual void connectToServer(const ServerProfile& profile,
                                 const QString& login,
                                 const QString& password) = 0;

    /// Requests a graceful disconnect. Emits disconnected() once torn down.
    virtual void disconnectFromServer() = 0;

    [[nodiscard]] virtual EngineState state() const = 0;
    [[nodiscard]] bool isConnected() const { return state() == EngineState::Connected; }

    /// Locates the vendor-provided executable this engine drives.
    /// Returns an empty string if it could not be found, in which case
    /// the engine cannot be used until the user installs the vendor
    /// software or points to it explicitly in Settings.
    [[nodiscard]] virtual QString locateExecutable() const = 0;

signals:
    void stateChanged(vpnofmes::core::EngineState state);
    void connected();
    void disconnected();
    void connectionFailed(const QString& reason);
    void logMessage(const QString& message);
};

} // namespace vpnofmes::core
