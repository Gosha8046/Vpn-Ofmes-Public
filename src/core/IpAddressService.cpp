#include "vpnofmes/core/IpAddressService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace vpnofmes::core {

IpAddressService::IpAddressService(QObject* parent)
    : QObject(parent), m_networkManager(this) {}

void IpAddressService::fetchPublicIp() {
    QNetworkRequest request(QUrl(QStringLiteral("https://api.ipify.org?format=json")));
    request.setTransferTimeout(5000);

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit ipFetchFailed(reply->errorString());
            return;
        }

        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        const QString ip = json.value("ip").toString();
        if (ip.isEmpty()) {
            emit ipFetchFailed(QStringLiteral("Malformed response from IP lookup service"));
            return;
        }

        emit ipFetched(ip);
    });
}

} // namespace vpnofmes::core
