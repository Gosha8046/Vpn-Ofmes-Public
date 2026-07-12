#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

namespace vpnofmes::core {

/// Checks a small JSON manifest published by the project (see
/// config/version.json) to find out whether a newer release of
/// Vpn-Ofmes is available.
///
/// Expected manifest shape:
/// {
///   "version": "1.1.0",
///   "url": "https://github.com/.../releases/latest",
///   "notes": "Bug fixes and performance improvements"
/// }
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QString currentVersion, QObject* parent = nullptr);

    /// Downloads the manifest at manifestUrl and compares it against the
    /// running version. Always emits exactly one of updateAvailable(),
    /// upToDate() or checkFailed().
    void checkForUpdates(const QString& manifestUrl);

signals:
    void updateAvailable(const QString& newVersion, const QString& downloadUrl, const QString& releaseNotes);
    void upToDate();
    void checkFailed(const QString& reason);

private:
    /// Compares two "x.y.z" version strings. Returns >0 if `a` is newer
    /// than `b`, <0 if older, 0 if equal. Missing/non-numeric components
    /// are treated as 0.
    static int compareVersions(const QString& a, const QString& b);

    QString m_currentVersion;
    QNetworkAccessManager m_networkManager;
};

} // namespace vpnofmes::core
