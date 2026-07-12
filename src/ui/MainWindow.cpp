#include "vpnofmes/ui/MainWindow.h"
#include "vpnofmes/Version.h"
#include "vpnofmes/core/Logger.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QVBoxLayout>

namespace vpnofmes::ui {

using vpnofmes::core::ConfigManager;
using vpnofmes::core::ConnectionManager;
using vpnofmes::core::ConnectionState;
using vpnofmes::core::CredentialStore;
using vpnofmes::core::IpAddressService;
using vpnofmes::core::LogLevel;
using vpnofmes::core::Logger;
using vpnofmes::core::NetworkMonitor;
using vpnofmes::core::ServerProfile;
using vpnofmes::core::UpdateChecker;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Vpn-Ofmes"));
    resize(880, 620);
    setMinimumSize(760, 560);

    // 1. Bring up configuration + logging before anything else touches them.
    m_configManager.initialize();
    Logger::instance().init(m_configManager.logsDir());
    m_configManager.load();
    Logger::instance().info(QStringLiteral("Vpn-Ofmes %1 starting up").arg(VPNOFMES_VERSION));

    m_credentialStore = new CredentialStore(
        QDir(m_configManager.writableConfigDir()).filePath("credentials.bin"));

    // 2. Core services.
    m_connectionManager = new ConnectionManager(m_configManager, this);
    m_networkMonitor = new NetworkMonitor(this);
    m_ipAddressService = new IpAddressService(this);
    m_updateChecker = new UpdateChecker(QStringLiteral(VPNOFMES_VERSION), this);

    // 3. UI.
    buildUi();
    applyTheme();
    wireServices();

    m_homePage->setServers(m_configManager.servers());
    if (!m_configManager.settings().lastSelectedServer.isEmpty()) {
        m_homePage->selectServerByName(m_configManager.settings().lastSelectedServer);
    }

    if (m_configManager.settings().rememberCredentials) {
        QString login, password;
        if (m_credentialStore->load(login, password)) {
            m_homePage->prefillCredentials(login, password, true);
        }
    }

    m_logsPage->loadHistory(Logger::instance().history());

    m_trayManager = new TrayManager(this);
    m_trayManager->show();
    connect(m_trayManager, &TrayManager::showMainWindowRequested, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    connect(m_trayManager, &TrayManager::connectRequested, this, [this]() {
        showNormal();
        navigateTo(0);
    });
    connect(m_trayManager, &TrayManager::disconnectRequested, this, &MainWindow::handleDisconnectRequested);
    connect(m_trayManager, &TrayManager::quitRequested, this, [this]() {
        m_forceQuit = true;
        close();
    });

    refreshIpBefore();

    if (m_configManager.settings().checkForUpdatesOnStartup) {
        checkForUpdates();
    }

    if (m_configManager.settings().startMinimizedToTray) {
        hide();
    } else {
        show();
    }
}

MainWindow::~MainWindow() {
    m_configManager.save();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    buildNavigationRail(central, rootLayout);

    m_stackedWidget = new QStackedWidget(central);
    m_homePage = new HomePage(m_stackedWidget);
    m_settingsPage = new SettingsPage(m_configManager, m_stackedWidget);
    m_logsPage = new LogsPage(m_stackedWidget);

    m_stackedWidget->addWidget(m_homePage);
    m_stackedWidget->addWidget(m_settingsPage);
    m_stackedWidget->addWidget(m_logsPage);

    rootLayout->addWidget(m_stackedWidget, 1);
    setCentralWidget(central);
}

void MainWindow::buildNavigationRail(QWidget* parent, QHBoxLayout* rootLayout) {
    auto* rail = new QWidget(parent);
    rail->setObjectName("navRail");
    rail->setFixedWidth(220);

    auto* railLayout = new QVBoxLayout(rail);
    railLayout->setContentsMargins(20, 28, 20, 20);
    railLayout->setSpacing(6);

    auto* brand = new QLabel(QStringLiteral("Vpn-Ofmes"), rail);
    brand->setObjectName("brandLabel");
    railLayout->addWidget(brand);
    railLayout->addSpacing(24);

    m_navHomeButton = new QPushButton(QStringLiteral("Главная"), rail);
    m_navSettingsButton = new QPushButton(QStringLiteral("Настройки"), rail);
    m_navLogsButton = new QPushButton(QStringLiteral("Логи"), rail);

    for (QPushButton* button : {m_navHomeButton, m_navSettingsButton, m_navLogsButton}) {
        button->setObjectName("navButton");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        railLayout->addWidget(button);
    }
    m_navHomeButton->setChecked(true);

    auto* group = new QButtonGroup(rail);
    group->setExclusive(true);
    group->addButton(m_navHomeButton, 0);
    group->addButton(m_navSettingsButton, 1);
    group->addButton(m_navLogsButton, 2);
    connect(group, &QButtonGroup::idClicked, this, &MainWindow::navigateTo);

    railLayout->addStretch();

    m_versionLabel = new QLabel(QStringLiteral("v%1").arg(VPNOFMES_VERSION), rail);
    m_versionLabel->setObjectName("versionLabel");
    railLayout->addWidget(m_versionLabel);

    rootLayout->addWidget(rail);
}

void MainWindow::navigateTo(int pageIndex) {
    if (pageIndex == m_stackedWidget->currentIndex()) {
        return;
    }
    m_stackedWidget->setCurrentIndex(pageIndex);

    // A gentle fade-in on every screen switch keeps the navigation feeling
    // alive without resorting to anything heavier.
    auto* effect = new QGraphicsOpacityEffect(m_stackedWidget->currentWidget());
    m_stackedWidget->currentWidget()->setGraphicsEffect(effect);
    auto* animation = new QPropertyAnimation(effect, "opacity", this);
    animation->setDuration(220);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QPropertyAnimation::finished, effect, &QObject::deleteLater);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::applyTheme() {
    const QString candidatePaths[] = {
        QDir(QApplication::applicationDirPath()).filePath("styles/dark_purple.qss"),
        QStringLiteral(":/styles/dark_purple.qss")
    };

    for (const QString& path : candidatePaths) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            setStyleSheet(QString::fromUtf8(file.readAll()));
            return;
        }
    }
    Logger::instance().warning(QStringLiteral("Could not locate dark_purple.qss, using default Qt style"));
}

void MainWindow::wireServices() {
    connect(m_homePage, &HomePage::connectRequested, this, &MainWindow::handleConnectRequested);
    connect(m_homePage, &HomePage::disconnectRequested, this, &MainWindow::handleDisconnectRequested);

    connect(m_connectionManager, &ConnectionManager::stateChanged, this, &MainWindow::handleConnectionStateChanged);
    connect(m_connectionManager, &ConnectionManager::logMessage, this,
            [](const QString& message) { Logger::instance().info(message); });

    connect(m_networkMonitor, &NetworkMonitor::speedUpdated, m_homePage, &HomePage::setSpeeds);

    connect(m_ipAddressService, &IpAddressService::ipFetched, m_homePage, &HomePage::setIpBefore);
    connect(m_ipAddressService, &IpAddressService::ipFetchFailed, this, [](const QString& reason) {
        Logger::instance().warning(QStringLiteral("Could not determine public IP: %1").arg(reason));
    });

    connect(&Logger::instance(), &Logger::entryAdded, m_logsPage, &LogsPage::appendEntry);

    connect(m_settingsPage, &SettingsPage::checkForUpdatesRequested, this, &MainWindow::checkForUpdates);

    connect(m_updateChecker, &UpdateChecker::updateAvailable, this,
            [this](const QString& version, const QString& url, const QString& notes) {
                const QString text = QStringLiteral("Update available: v%1").arg(version);
                m_settingsPage->setUpdateStatusText(text);
                Logger::instance().info(QStringLiteral("%1 - %2 (%3)").arg(text, notes, url));
                m_trayManager->showMessage(QStringLiteral("Vpn-Ofmes"), text);
            });
    connect(m_updateChecker, &UpdateChecker::upToDate, this, [this]() {
        m_settingsPage->setUpdateStatusText(QStringLiteral("Up to date"));
    });
    connect(m_updateChecker, &UpdateChecker::checkFailed, this, [this](const QString& reason) {
        m_settingsPage->setUpdateStatusText(QStringLiteral("Update check failed"));
        Logger::instance().warning(QStringLiteral("Update check failed: %1").arg(reason));
    });
}

void MainWindow::handleConnectRequested(const ServerProfile& profile,
                                        const QString& login,
                                        const QString& password,
                                        bool rememberCredentials) {
    m_configManager.settings().lastSelectedServer = profile.name();
    m_configManager.settings().rememberCredentials = rememberCredentials;
    m_configManager.save();

    if (rememberCredentials) {
        m_credentialStore->save(login, password);
    } else {
        m_credentialStore->clear();
    }

    m_connectionManager->connectTo(profile, login, password);
}

void MainWindow::handleDisconnectRequested() {
    m_connectionManager->disconnectFromVpn();
}

void MainWindow::handleConnectionStateChanged(ConnectionState state) {
    m_homePage->setConnectionState(state);
    m_trayManager->setConnectionState(state);

    if (state == ConnectionState::Connected) {
        m_networkMonitor->start();
        refreshIpAfter();
    } else if (state == ConnectionState::Disconnected) {
        m_networkMonitor->stop();
        m_homePage->setSpeeds(0.0, 0.0);
        m_homePage->setIpAfter(QStringLiteral("—"));
        refreshIpBefore();
    }
}

void MainWindow::refreshIpBefore() {
    // Signal/slot wiring for m_ipAddressService is done once in
    // wireServices(); this just kicks off another lookup.
    m_ipAddressService->fetchPublicIp();
}

void MainWindow::refreshIpAfter() {
    auto* service = new IpAddressService(this);
    connect(service, &IpAddressService::ipFetched, this, [this, service](const QString& ip) {
        m_homePage->setIpAfter(ip);
        service->deleteLater();
    });
    connect(service, &IpAddressService::ipFetchFailed, this, [service](const QString& reason) {
        Logger::instance().warning(QStringLiteral("Could not determine post-connect public IP: %1").arg(reason));
        service->deleteLater();
    });
    service->fetchPublicIp();
}

void MainWindow::checkForUpdates() {
    m_settingsPage->setUpdateStatusText(QStringLiteral("Checking..."));
    m_updateChecker->checkForUpdates(m_configManager.settings().updateManifestUrl);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_forceQuit && m_configManager.settings().minimizeToTrayOnClose) {
        event->ignore();
        hide();
        m_trayManager->showMessage(QStringLiteral("Vpn-Ofmes"),
                                    QStringLiteral("Still running in the background"));
        return;
    }

    m_configManager.save();
    event->accept();
    QApplication::quit();
}

} // namespace vpnofmes::ui
