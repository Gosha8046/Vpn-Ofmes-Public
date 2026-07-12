#pragma once

#include <QString>
#include <QJsonObject>

namespace vpnofmes::core {

/// Which underlying VPN engine a profile should be started with.
/// Vpn-Ofmes never implements its own tunneling/crypto -- it always
/// delegates to one of these officially maintained, open-source engines.
enum class VpnProtocol {
    WireGuard,
    OpenVpn
};

QString protocolToString(VpnProtocol protocol);
VpnProtocol protocolFromString(const QString& text);

/// Immutable-ish value object describing a single VPN server the user can
/// connect to. Holds only connection metadata -- credentials are kept out
/// of this struct and managed separately by CredentialStore so that server
/// lists can be exported/shared without leaking secrets.
class ServerProfile {
public:
    ServerProfile() = default;
    ServerProfile(QString name, QString host, quint16 port, VpnProtocol protocol);

    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] QString host() const { return m_host; }
    [[nodiscard]] quint16 port() const { return m_port; }
    [[nodiscard]] VpnProtocol protocol() const { return m_protocol; }
    /// Path to the engine-specific config template (.conf for WireGuard,
    /// .ovpn for OpenVPN) shipped alongside the profile, if any.
    [[nodiscard]] QString configTemplatePath() const { return m_configTemplatePath; }

    void setName(const QString& name) { m_name = name; }
    void setHost(const QString& host) { m_host = host; }
    void setPort(quint16 port) { m_port = port; }
    void setProtocol(VpnProtocol protocol) { m_protocol = protocol; }
    void setConfigTemplatePath(const QString& path) { m_configTemplatePath = path; }

    [[nodiscard]] QJsonObject toJson() const;
    static ServerProfile fromJson(const QJsonObject& json);

private:
    QString m_name;
    QString m_host;
    quint16 m_port = 51820;
    VpnProtocol m_protocol = VpnProtocol::WireGuard;
    QString m_configTemplatePath;
};

} // namespace vpnofmes::core
