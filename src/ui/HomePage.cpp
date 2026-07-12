#include "vpnofmes/ui/HomePage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QSpacerItem>

namespace vpnofmes::ui {

using vpnofmes::core::ConnectionState;
using vpnofmes::core::ServerProfile;
using vpnofmes::core::VpnProtocol;

HomePage::HomePage(QWidget* parent) : QWidget(parent) {
    buildUi();
}

void HomePage::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 28, 32, 28);
    rootLayout->setSpacing(20);

    // --- Server selection card ---------------------------------------
    auto* serverCard = new QFrame(this);
    serverCard->setObjectName("card");
    auto* serverLayout = new QGridLayout(serverCard);
    serverLayout->setSpacing(10);

    auto* serverTitle = new QLabel(QStringLiteral("Server"), serverCard);
    serverTitle->setObjectName("cardTitle");
    serverLayout->addWidget(serverTitle, 0, 0, 1, 2);

    m_serverCombo = new QComboBox(serverCard);
    serverLayout->addWidget(m_serverCombo, 1, 0, 1, 2);

    serverLayout->addWidget(new QLabel(QStringLiteral("Address"), serverCard), 2, 0);
    m_addressEdit = new QLineEdit(serverCard);
    m_addressEdit->setPlaceholderText(QStringLiteral("vpn.example.com"));
    serverLayout->addWidget(m_addressEdit, 3, 0, 1, 2);

    rootLayout->addWidget(serverCard);

    // --- Credentials card ----------------------------------------------
    auto* credentialsCard = new QFrame(this);
    credentialsCard->setObjectName("card");
    auto* credentialsLayout = new QGridLayout(credentialsCard);
    credentialsLayout->setSpacing(10);

    auto* credentialsTitle = new QLabel(QStringLiteral("Credentials"), credentialsCard);
    credentialsTitle->setObjectName("cardTitle");
    credentialsLayout->addWidget(credentialsTitle, 0, 0, 1, 2);

    credentialsLayout->addWidget(new QLabel(QStringLiteral("Login"), credentialsCard), 1, 0);
    m_loginEdit = new QLineEdit(credentialsCard);
    m_loginEdit->setPlaceholderText(QStringLiteral("username"));
    credentialsLayout->addWidget(m_loginEdit, 1, 1);

    credentialsLayout->addWidget(new QLabel(QStringLiteral("Password"), credentialsCard), 2, 0);
    m_passwordEdit = new QLineEdit(credentialsCard);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    credentialsLayout->addWidget(m_passwordEdit, 2, 1);

    m_rememberCheckbox = new QCheckBox(QStringLiteral("Remember credentials on this device"), credentialsCard);
    credentialsLayout->addWidget(m_rememberCheckbox, 3, 0, 1, 2);

    rootLayout->addWidget(credentialsCard);

    // --- Status / actions card ------------------------------------------
    auto* statusCard = new QFrame(this);
    statusCard->setObjectName("card");
    auto* statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setSpacing(14);

    auto* statusRow = new QHBoxLayout();
    m_statusIndicator = new StatusIndicator(statusCard);
    m_statusLabel = new QLabel(QStringLiteral("Disconnected"), statusCard);
    m_statusLabel->setObjectName("statusLabel");
    statusRow->addWidget(m_statusIndicator);
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch();
    statusLayout->addLayout(statusRow);

    m_speedWidget = new SpeedWidget(statusCard);
    statusLayout->addWidget(m_speedWidget);

    auto* ipRow = new QHBoxLayout();
    m_ipBeforeLabel = new QLabel(QStringLiteral("Public IP before: —"), statusCard);
    m_ipAfterLabel = new QLabel(QStringLiteral("Public IP after: —"), statusCard);
    ipRow->addWidget(m_ipBeforeLabel);
    ipRow->addWidget(m_ipAfterLabel);
    ipRow->addStretch();
    statusLayout->addLayout(ipRow);

    auto* buttonRow = new QHBoxLayout();
    m_connectButton = new QPushButton(QStringLiteral("Подключиться"), statusCard);
    m_connectButton->setObjectName("primaryButton");
    m_disconnectButton = new QPushButton(QStringLiteral("Отключиться"), statusCard);
    m_disconnectButton->setObjectName("secondaryButton");
    m_disconnectButton->setEnabled(false);
    buttonRow->addWidget(m_connectButton);
    buttonRow->addWidget(m_disconnectButton);
    statusLayout->addLayout(buttonRow);

    rootLayout->addWidget(statusCard);
    rootLayout->addStretch();

    connect(m_serverCombo, &QComboBox::currentIndexChanged, this, &HomePage::onServerSelectionChanged);
    connect(m_connectButton, &QPushButton::clicked, this, &HomePage::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &HomePage::disconnectRequested);
}

void HomePage::setServers(const QVector<ServerProfile>& servers) {
    m_servers = servers;
    m_serverCombo->blockSignals(true);
    m_serverCombo->clear();
    for (const ServerProfile& profile : servers) {
        m_serverCombo->addItem(profile.name());
    }
    m_serverCombo->blockSignals(false);

    if (!servers.isEmpty()) {
        onServerSelectionChanged(0);
    }
}

void HomePage::selectServerByName(const QString& name) {
    const int index = m_serverCombo->findText(name);
    if (index >= 0) {
        m_serverCombo->setCurrentIndex(index);
    }
}

void HomePage::onServerSelectionChanged(int index) {
    if (index < 0 || index >= m_servers.size()) {
        return;
    }
    const ServerProfile& profile = m_servers.at(index);
    m_addressEdit->setText(QStringLiteral("%1:%2").arg(profile.host()).arg(profile.port()));

    // WireGuard authenticates with keys baked into its config template,
    // not username/password, so those fields are not applicable to it.
    const bool credentialsApply = (profile.protocol() == VpnProtocol::OpenVpn);
    m_loginEdit->setEnabled(credentialsApply);
    m_passwordEdit->setEnabled(credentialsApply);
    m_rememberCheckbox->setEnabled(credentialsApply);
    m_loginEdit->setPlaceholderText(credentialsApply
        ? QStringLiteral("username")
        : QStringLiteral("not required for WireGuard"));
}

void HomePage::onConnectClicked() {
    const int index = m_serverCombo->currentIndex();
    if (index < 0 || index >= m_servers.size()) {
        return;
    }
    emit connectRequested(m_servers.at(index), m_loginEdit->text(), m_passwordEdit->text(),
                           m_rememberCheckbox->isChecked());
}

void HomePage::setConnectionState(ConnectionState state) {
    m_state = state;
    m_statusIndicator->setConnectionState(state);

    switch (state) {
        case ConnectionState::Disconnected:
            m_statusLabel->setText(QStringLiteral("Disconnected"));
            m_connectButton->setEnabled(true);
            m_disconnectButton->setEnabled(false);
            m_serverCombo->setEnabled(true);
            break;
        case ConnectionState::Connecting:
            m_statusLabel->setText(QStringLiteral("Connecting..."));
            m_connectButton->setEnabled(false);
            m_disconnectButton->setEnabled(true);
            m_serverCombo->setEnabled(false);
            break;
        case ConnectionState::Connected:
            m_statusLabel->setText(QStringLiteral("Connected"));
            m_connectButton->setEnabled(false);
            m_disconnectButton->setEnabled(true);
            m_serverCombo->setEnabled(false);
            break;
        case ConnectionState::Reconnecting:
            m_statusLabel->setText(QStringLiteral("Reconnecting..."));
            m_connectButton->setEnabled(false);
            m_disconnectButton->setEnabled(true);
            m_serverCombo->setEnabled(false);
            break;
        case ConnectionState::Error:
            m_statusLabel->setText(QStringLiteral("Connection error"));
            m_connectButton->setEnabled(true);
            m_disconnectButton->setEnabled(false);
            m_serverCombo->setEnabled(true);
            break;
    }
}

void HomePage::setSpeeds(double downloadKBps, double uploadKBps) {
    m_speedWidget->updateSpeeds(downloadKBps, uploadKBps);
}

void HomePage::setIpBefore(const QString& ip) {
    m_ipBeforeLabel->setText(QStringLiteral("Public IP before: %1").arg(ip));
}

void HomePage::setIpAfter(const QString& ip) {
    m_ipAfterLabel->setText(QStringLiteral("Public IP after: %1").arg(ip));
}

void HomePage::prefillCredentials(const QString& login, const QString& password, bool remember) {
    m_loginEdit->setText(login);
    m_passwordEdit->setText(password);
    m_rememberCheckbox->setChecked(remember);
}

} // namespace vpnofmes::ui
