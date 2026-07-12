#include "vpnofmes/core/UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace vpnofmes::core {

UpdateChecker::UpdateChecker(QString currentVersion, QObject* parent)
    : QObject(parent), m_currentVersion(std::move(currentVersion)), m_networkManager(this) {}

int UpdateChecker::compareVersions(const QString& a, const QString& b) {
    const QStringList partsA = a.split('.');
    const QStringList partsB = b.split('.');
    const int count = std::max(partsA.size(), partsB.size());

    for (int i = 0; i < count; ++i) {
        const int valueA = i < partsA.size() ? partsA.at(i).toInt() : 0;
        const int valueB = i < partsB.size() ? partsB.at(i).toInt() : 0;
        if (valueA != valueB) {
            return valueA - valueB;
        }
    }
    return 0;
}

void UpdateChecker::checkForUpdates(const QString& manifestUrl) {
    QNetworkRequest request((QUrl(manifestUrl)));
    request.setTransferTimeout(7000);

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit checkFailed(reply->errorString());
            return;
        }

        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        const QString remoteVersion = json.value("version").toString();
        if (remoteVersion.isEmpty()) {
            emit checkFailed(QStringLiteral("Update manifest did not contain a version field"));
            return;
        }

        if (compareVersions(remoteVersion, m_currentVersion) > 0) {
            emit updateAvailable(remoteVersion,
                                  json.value("url").toString(),
                                  json.value("notes").toString());
        } else {
            emit upToDate();
        }
    });
}

} // namespace vpnofmes::core
