#pragma once

#include <QString>

namespace vpnofmes::core {

/// Persists the VPN login/password pair the user optionally chooses to
/// "remember", protected at rest.
///
/// On Windows the secret is encrypted with the Data Protection API
/// (CryptProtectData / CryptUnprotectData). DPAPI ties the encryption key
/// to the logged-in Windows user account, so the encrypted blob on disk is
/// useless to anyone without that user's Windows session -- this is the
/// same mechanism Chrome/Edge use to protect saved passwords. No custom
/// cryptography is implemented here, only calls into the OS-provided API.
///
/// On non-Windows platforms (used for local development/compile checks of
/// the rest of the application) the store falls back to Base64 obfuscation
/// and is NOT considered secure; it exists purely so the class remains
/// usable while iterating on other modules away from Windows.
class CredentialStore {
public:
    explicit CredentialStore(QString filePath);

    /// Encrypts and writes {login, password} to disk.
    bool save(const QString& login, const QString& password) const;

    /// Reads and decrypts the previously saved credentials. Returns false
    /// if nothing has been saved yet or decryption fails (e.g. the file
    /// was copied to a different machine/user).
    bool load(QString& outLogin, QString& outPassword) const;

    /// Deletes any saved credentials.
    void clear() const;

    [[nodiscard]] bool hasSavedCredentials() const;

private:
    QString m_filePath;
};

} // namespace vpnofmes::core
