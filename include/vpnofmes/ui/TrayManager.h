#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

#include "vpnofmes/core/ConnectionManager.h"

namespace vpnofmes::ui {

/// Wraps QSystemTrayIcon: minimizing the main window hides it into the
/// Windows notification area, with a context menu for the most common
/// actions and an icon that reflects the current connection state.
class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(QObject* parent = nullptr);

    void show();
    void setConnectionState(vpnofmes::core::ConnectionState state);
    void showMessage(const QString& title, const QString& message);

signals:
    void showMainWindowRequested();
    void connectRequested();
    void disconnectRequested();
    void quitRequested();

private:
    void buildMenu();
    void updateIcon();

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_menu = nullptr;
    QAction* m_connectAction = nullptr;
    QAction* m_disconnectAction = nullptr;
    QAction* m_statusAction = nullptr;
    vpnofmes::core::ConnectionState m_state = vpnofmes::core::ConnectionState::Disconnected;
};

} // namespace vpnofmes::ui
