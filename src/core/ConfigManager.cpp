#include "vpnofmes/core/ConfigManager.h"
#include "vpnofmes/core/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace vpnofmes::core {

namespace {
constexpr auto kSettingsFileName = "app_config.json";
constexpr auto kServersFileName = "servers.json";
} // namespace

ConfigManager::ConfigManager() = default;

void ConfigManager::initialize() {
    m_installDir = QCoreApplication::applicationDirPath();
    m_writableDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QDir dir(m_writableDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    seedDefaultsIfMissing();
}

QString ConfigManager::logsDir() const {
    return QDir(m_writableDir).filePath("logs");
}

void ConfigManager::seedDefaultsIfMissing() {
    const QString settingsPath = QDir(m_writableDir).filePath(kSettingsFileName);
    const QString serversPath = QDir(m_writableDir).filePath(kServersFileName);

    if (!QFile::exists(settingsPath)) {
        const QString defaultSettings = QDir(m_installDir).filePath("config/app_config.default.json");
        if (QFile::exists(defaultSettings)) {
            QFile::copy(defaultSettings, settingsPath);
            QFile::setPermissions(settingsPath, QFile::ReadOwner | QFile::WriteOwner);
        }
    }

    if (!QFile::exists(serversPath)) {
        const QString defaultServers = QDir(m_installDir).filePath("config/servers.default.json");
        if (QFile::exists(defaultServers)) {
            QFile::copy(defaultServers, serversPath);
            QFile::setPermissions(serversPath, QFile::ReadOwner | QFile::WriteOwner);
        }
    }
}

void ConfigManager::load() {
    loadSettingsFile();
    loadServersFile();
}

void ConfigManager::loadSettingsFile() {
    const QString path = QDir(m_writableDir).filePath(kSettingsFileName);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().warning(QStringLiteral("No settings file found at %1, using defaults").arg(path));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject json = doc.object();

    m_settings.startMinimizedToTray = json.value("startMinimizedToTray").toBool(m_settings.startMinimizedToTray);
    m_settings.minimizeToTrayOnClose = json.value("minimizeToTrayOnClose").toBool(m_settings.minimizeToTrayOnClose);
    m_settings.autoReconnect = json.value("autoReconnect").toBool(m_settings.autoReconnect);
    m_settings.reconnectIntervalSeconds = json.value("reconnectIntervalSeconds").toInt(m_settings.reconnectIntervalSeconds);
    m_settings.maxReconnectAttempts = json.value("maxReconnectAttempts").toInt(m_settings.maxReconnectAttempts);
    m_settings.checkForUpdatesOnStartup = json.value("checkForUpdatesOnStartup").toBool(m_settings.checkForUpdatesOnStartup);
    m_settings.updateManifestUrl = json.value("updateManifestUrl").toString(m_settings.updateManifestUrl);
    m_settings.lastSelectedServer = json.value("lastSelectedServer").toString();
    m_settings.rememberCredentials = json.value("rememberCredentials").toBool(m_settings.rememberCredentials);
}

void ConfigManager::loadServersFile() {
    const QString path = QDir(m_writableDir).filePath(kServersFileName);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().warning(QStringLiteral("No servers file found at %1").arg(path));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    m_servers.clear();
    for (const QJsonValue& value : doc.array()) {
        ServerProfile profile = ServerProfile::fromJson(value.toObject());

        // Config template paths in the bundled defaults are relative to the
        // installation directory; resolve them to absolute paths once so
        // the VPN engines can open them regardless of the process's
        // current working directory.
        const QString templatePath = profile.configTemplatePath();
        if (!templatePath.isEmpty() && QDir::isRelativePath(templatePath)) {
            profile.setConfigTemplatePath(QDir(m_installDir).filePath(templatePath));
        }

        m_servers.append(profile);
    }
}

void ConfigManager::save() const {
    saveSettingsFile();
    saveServersFile();
}

void ConfigManager::saveSettingsFile() const {
    QJsonObject json;
    json["startMinimizedToTray"] = m_settings.startMinimizedToTray;
    json["minimizeToTrayOnClose"] = m_settings.minimizeToTrayOnClose;
    json["autoReconnect"] = m_settings.autoReconnect;
    json["reconnectIntervalSeconds"] = m_settings.reconnectIntervalSeconds;
    json["maxReconnectAttempts"] = m_settings.maxReconnectAttempts;
    json["checkForUpdatesOnStartup"] = m_settings.checkForUpdatesOnStartup;
    json["updateManifestUrl"] = m_settings.updateManifestUrl;
    json["lastSelectedServer"] = m_settings.lastSelectedServer;
    json["rememberCredentials"] = m_settings.rememberCredentials;

    QFile file(QDir(m_writableDir).filePath(kSettingsFileName));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
}

void ConfigManager::saveServersFile() const {
    QJsonArray array;
    for (const ServerProfile& profile : m_servers) {
        array.append(profile.toJson());
    }

    QFile file(QDir(m_writableDir).filePath(kServersFileName));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

void ConfigManager::addServer(const ServerProfile& profile) {
    for (ServerProfile& existing : m_servers) {
        if (existing.name() == profile.name()) {
            existing = profile;
            return;
        }
    }
    m_servers.append(profile);
}

void ConfigManager::removeServer(const QString& name) {
    for (int i = 0; i < m_servers.size(); ++i) {
        if (m_servers[i].name() == name) {
            m_servers.remove(i);
            return;
        }
    }
}

} // namespace vpnofmes::core
