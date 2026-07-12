#pragma once

#include <QObject>
#include <QNetworkAccessManager>

namespace vpnofmes::core {

/// Looks up the machine's current public IP address by asking a public
/// HTTP endpoint, so the UI can show "IP before connecting" vs
/// "IP after connecting" side by side. This performs a plain read-only
/// HTTPS GET request; no data about the user is sent beyond the request
/// itself.
class IpAddressService : public QObject {
    Q_OBJECT

public:
    explicit IpAddressService(QObject* parent = nullptr);

    /// Asynchronously fetches the current public IP. Emits ipFetched() on
    /// success or ipFetchFailed() if the request could not be completed
    /// (e.g. no connectivity while the tunnel is establishing).
    void fetchPublicIp();

signals:
    void ipFetched(const QString& ipAddress);
    void ipFetchFailed(const QString& reason);

private:
    QNetworkAccessManager m_networkManager;
};

} // namespace vpnofmes::core
