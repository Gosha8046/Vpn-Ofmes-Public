#include "vpnofmes/core/ServerProfile.h"

namespace vpnofmes::core {

QString protocolToString(VpnProtocol protocol) {
    switch (protocol) {
        case VpnProtocol::WireGuard: return QStringLiteral("wireguard");
        case VpnProtocol::OpenVpn:   return QStringLiteral("openvpn");
    }
    return QStringLiteral("wireguard");
}

VpnProtocol protocolFromString(const QString& text) {
    if (text.compare(QStringLiteral("openvpn"), Qt::CaseInsensitive) == 0) {
        return VpnProtocol::OpenVpn;
    }
    return VpnProtocol::WireGuard;
}

ServerProfile::ServerProfile(QString name, QString host, quint16 port, VpnProtocol protocol)
    : m_name(std::move(name)),
      m_host(std::move(host)),
      m_port(port),
      m_protocol(protocol) {}

QJsonObject ServerProfile::toJson() const {
    QJsonObject json;
    json["name"] = m_name;
    json["host"] = m_host;
    json["port"] = m_port;
    json["protocol"] = protocolToString(m_protocol);
    json["configTemplatePath"] = m_configTemplatePath;
    return json;
}

ServerProfile ServerProfile::fromJson(const QJsonObject& json) {
    ServerProfile profile;
    profile.m_name = json.value("name").toString();
    profile.m_host = json.value("host").toString();
    profile.m_port = static_cast<quint16>(json.value("port").toInt(51820));
    profile.m_protocol = protocolFromString(json.value("protocol").toString());
    profile.m_configTemplatePath = json.value("configTemplatePath").toString();
    return profile;
}

} // namespace vpnofmes::core
