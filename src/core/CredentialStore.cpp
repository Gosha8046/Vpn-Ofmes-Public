#include "vpnofmes/core/CredentialStore.h"
#include "vpnofmes/core/Logger.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace vpnofmes::core {

namespace {

#ifdef Q_OS_WIN
/// Encrypts a byte array for the current Windows user via DPAPI.
QByteArray dpapiProtect(const QByteArray& plain) {
    DATA_BLOB in{};
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.constData()));
    in.cbData = static_cast<DWORD>(plain.size());

    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"VpnOfmesCredentials", nullptr, nullptr, nullptr,
                           CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        return {};
    }

    QByteArray result(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return result;
}

/// Decrypts a DPAPI-protected byte array. Only succeeds for the same
/// Windows user account that encrypted it.
QByteArray dpapiUnprotect(const QByteArray& cipher) {
    DATA_BLOB in{};
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(cipher.constData()));
    in.cbData = static_cast<DWORD>(cipher.size());

    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr,
                             CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        return {};
    }

    QByteArray result(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return result;
}
#endif

} // namespace

CredentialStore::CredentialStore(QString filePath) : m_filePath(std::move(filePath)) {}

bool CredentialStore::save(const QString& login, const QString& password) const {
    QJsonObject payload;
    payload["login"] = login;
    payload["password"] = password;
    const QByteArray plain = QJsonDocument(payload).toJson(QJsonDocument::Compact);

#ifdef Q_OS_WIN
    const QByteArray protectedBytes = dpapiProtect(plain);
    if (protectedBytes.isEmpty()) {
        Logger::instance().error(QStringLiteral("Failed to protect credentials with DPAPI"));
        return false;
    }
#else
    // Development-only fallback, not secure. Real deployments target Windows.
    const QByteArray protectedBytes = plain.toBase64();
#endif

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(protectedBytes);
    return true;
}

bool CredentialStore::load(QString& outLogin, QString& outPassword) const {
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray stored = file.readAll();

#ifdef Q_OS_WIN
    const QByteArray plain = dpapiUnprotect(stored);
#else
    const QByteArray plain = QByteArray::fromBase64(stored);
#endif

    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject payload = QJsonDocument::fromJson(plain).object();
    outLogin = payload.value("login").toString();
    outPassword = payload.value("password").toString();
    return true;
}

void CredentialStore::clear() const {
    QFile::remove(m_filePath);
}

bool CredentialStore::hasSavedCredentials() const {
    return QFile::exists(m_filePath);
}

} // namespace vpnofmes::core
