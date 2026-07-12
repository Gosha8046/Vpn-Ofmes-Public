#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>

#include "vpnofmes/core/ConfigManager.h"
#include "vpnofmes/core/ConnectionManager.h"
#include "vpnofmes/core/CredentialStore.h"
#include "vpnofmes/core/IpAddressService.h"
#include "vpnofmes/core/NetworkMonitor.h"
#include "vpnofmes/core/UpdateChecker.h"
#include "vpnofmes/ui/HomePage.h"
#include "vpnofmes/ui/LogsPage.h"
#include "vpnofmes/ui/SettingsPage.h"
#include "vpnofmes/ui/TrayManager.h"

namespace vpnofmes::ui {

/// Top-level application window: a navigation rail on the left (Home /
/// Settings / Logs) and a QStackedWidget holding the three screens on the
/// right. Owns every core service and wires their signals to the pages,
/// acting as the composition root of the application.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void buildNavigationRail(QWidget* parent, QHBoxLayout* rootLayout);
    void wireServices();
    void applyTheme();
    void navigateTo(int pageIndex);

    void handleConnectRequested(const vpnofmes::core::ServerProfile& profile,
                                 const QString& login,
                                 const QString& password,
                                 bool rememberCredentials);
    void handleDisconnectRequested();
    void handleConnectionStateChanged(vpnofmes::core::ConnectionState state);
    void refreshIpBefore();
    void refreshIpAfter();
    void checkForUpdates();

    // --- Core services (composition root) -------------------------------
    vpnofmes::core::ConfigManager m_configManager;
    vpnofmes::core::ConnectionManager* m_connectionManager = nullptr;
    vpnofmes::core::NetworkMonitor* m_networkMonitor = nullptr;
    vpnofmes::core::IpAddressService* m_ipAddressService = nullptr;
    vpnofmes::core::UpdateChecker* m_updateChecker = nullptr;
    vpnofmes::core::CredentialStore* m_credentialStore = nullptr;

    // --- UI ---------------------------------------------------------------
    QStackedWidget* m_stackedWidget = nullptr;
    HomePage* m_homePage = nullptr;
    SettingsPage* m_settingsPage = nullptr;
    LogsPage* m_logsPage = nullptr;
    TrayManager* m_trayManager = nullptr;

    QPushButton* m_navHomeButton = nullptr;
    QPushButton* m_navSettingsButton = nullptr;
    QPushButton* m_navLogsButton = nullptr;
    QLabel* m_versionLabel = nullptr;

    bool m_forceQuit = false;
};

} // namespace vpnofmes::ui
