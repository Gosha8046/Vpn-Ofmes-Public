#pragma once

#include <QString>
#include <QVector>

#include "vpnofmes/core/ServerProfile.h"

namespace vpnofmes::core {

/// User-configurable application settings (everything shown on the
/// Settings screen except server profiles, which are handled separately).
struct AppSettings {
    bool startMinimizedToTray = false;
    bool minimizeToTrayOnClose = true;
    bool autoReconnect = true;
    int  reconnectIntervalSeconds = 5;
    int  maxReconnectAttempts = 10;
    bool checkForUpdatesOnStartup = true;
    QString updateManifestUrl = QStringLiteral(
        "https://raw.githubusercontent.com/gosha8046/vpn-ofmes-public/main/config/version.json");
    QString lastSelectedServer;
    bool rememberCredentials = false;
};

/// Loads and persists application configuration.
///
/// On first launch the manager copies the read-only defaults that ship
/// next to the executable (config/app_config.default.json and
/// config/servers.default.json) into a writable per-user location
/// (%APPDATA%/Vpn-Ofmes on Windows). Every subsequent run reads and writes
/// that writable copy, so the installation directory never needs to be
/// writable and settings survive application updates.
class ConfigManager {
public:
    ConfigManager();

    /// Prepares the writable config directory, seeding it from the
    /// bundled defaults if this is the first run. Must be called once at
    /// start-up before load().
    void initialize();

    void load();
    void save() const;

    [[nodiscard]] AppSettings& settings() { return m_settings; }
    [[nodiscard]] const AppSettings& settings() const { return m_settings; }

    [[nodiscard]] QVector<ServerProfile>& servers() { return m_servers; }
    [[nodiscard]] const QVector<ServerProfile>& servers() const { return m_servers; }

    void addServer(const ServerProfile& profile);
    void removeServer(const QString& name);

    [[nodiscard]] QString writableConfigDir() const { return m_writableDir; }
    [[nodiscard]] QString logsDir() const;

private:
    void seedDefaultsIfMissing();
    void loadSettingsFile();
    void loadServersFile();
    void saveSettingsFile() const;
    void saveServersFile() const;

    QString m_writableDir;
    QString m_installDir;
    AppSettings m_settings;
    QVector<ServerProfile> m_servers;
};

} // namespace vpnofmes::core
