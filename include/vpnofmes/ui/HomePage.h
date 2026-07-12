#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVector>

#include "vpnofmes/core/ConnectionManager.h"
#include "vpnofmes/core/ServerProfile.h"
#include "vpnofmes/ui/SpeedWidget.h"
#include "vpnofmes/ui/StatusIndicator.h"

namespace vpnofmes::ui {

/// The main screen: server selection, address/credentials, the big
/// Connect/Disconnect button, connection status, live throughput and the
/// public-IP-before/after comparison.
class HomePage : public QWidget {
    Q_OBJECT

public:
    explicit HomePage(QWidget* parent = nullptr);

    /// Refreshes the server drop-down from the current profile list.
    void setServers(const QVector<vpnofmes::core::ServerProfile>& servers);
    void selectServerByName(const QString& name);

    void setConnectionState(vpnofmes::core::ConnectionState state);
    void setSpeeds(double downloadKBps, double uploadKBps);
    void setIpBefore(const QString& ip);
    void setIpAfter(const QString& ip);
    void prefillCredentials(const QString& login, const QString& password, bool remember);

signals:
    void connectRequested(const vpnofmes::core::ServerProfile& profile,
                           const QString& login,
                           const QString& password,
                           bool rememberCredentials);
    void disconnectRequested();

private:
    void buildUi();
    void onServerSelectionChanged(int index);
    void onConnectClicked();

    QComboBox* m_serverCombo = nullptr;
    QLineEdit* m_addressEdit = nullptr;
    QLineEdit* m_loginEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QCheckBox* m_rememberCheckbox = nullptr;
    QPushButton* m_connectButton = nullptr;
    QPushButton* m_disconnectButton = nullptr;

    StatusIndicator* m_statusIndicator = nullptr;
    QLabel* m_statusLabel = nullptr;
    SpeedWidget* m_speedWidget = nullptr;

    QLabel* m_ipBeforeLabel = nullptr;
    QLabel* m_ipAfterLabel = nullptr;

    QVector<vpnofmes::core::ServerProfile> m_servers;
    vpnofmes::core::ConnectionState m_state = vpnofmes::core::ConnectionState::Disconnected;
};

} // namespace vpnofmes::ui
