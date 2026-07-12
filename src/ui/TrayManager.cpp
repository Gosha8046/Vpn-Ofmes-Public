#include "vpnofmes/ui/TrayManager.h"

namespace vpnofmes::ui {

using vpnofmes::core::ConnectionState;

TrayManager::TrayManager(QObject* parent) : QObject(parent) {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip(QStringLiteral("Vpn-Ofmes"));

    buildMenu();
    updateIcon();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            emit showMainWindowRequested();
        }
    });
}

void TrayManager::buildMenu() {
    m_menu = new QMenu();

    m_statusAction = m_menu->addAction(QStringLiteral("Status: Disconnected"));
    m_statusAction->setEnabled(false);
    m_menu->addSeparator();

    auto* showAction = m_menu->addAction(QStringLiteral("Open Vpn-Ofmes"));
    connect(showAction, &QAction::triggered, this, &TrayManager::showMainWindowRequested);

    m_connectAction = m_menu->addAction(QStringLiteral("Connect"));
    connect(m_connectAction, &QAction::triggered, this, &TrayManager::connectRequested);

    m_disconnectAction = m_menu->addAction(QStringLiteral("Disconnect"));
    connect(m_disconnectAction, &QAction::triggered, this, &TrayManager::disconnectRequested);

    m_menu->addSeparator();
    auto* quitAction = m_menu->addAction(QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, this, &TrayManager::quitRequested);

    m_trayIcon->setContextMenu(m_menu);
}

void TrayManager::show() {
    m_trayIcon->show();
}

void TrayManager::updateIcon() {
    const bool connected = (m_state == ConnectionState::Connected);
    const QString iconPath = connected ? QStringLiteral(":/icons/tray_connected.svg")
                                        : QStringLiteral(":/icons/tray_disconnected.svg");
    m_trayIcon->setIcon(QIcon(iconPath));
}

void TrayManager::setConnectionState(ConnectionState state) {
    m_state = state;
    updateIcon();

    QString label;
    switch (state) {
        case ConnectionState::Disconnected:  label = QStringLiteral("Status: Disconnected"); break;
        case ConnectionState::Connecting:    label = QStringLiteral("Status: Connecting..."); break;
        case ConnectionState::Connected:     label = QStringLiteral("Status: Connected"); break;
        case ConnectionState::Reconnecting:  label = QStringLiteral("Status: Reconnecting..."); break;
        case ConnectionState::Error:         label = QStringLiteral("Status: Error"); break;
    }
    m_statusAction->setText(label);

    const bool connected = (state == ConnectionState::Connected || state == ConnectionState::Connecting ||
                             state == ConnectionState::Reconnecting);
    m_connectAction->setEnabled(!connected);
    m_disconnectAction->setEnabled(connected);
}

void TrayManager::showMessage(const QString& title, const QString& message) {
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 4000);
}

} // namespace vpnofmes::ui
