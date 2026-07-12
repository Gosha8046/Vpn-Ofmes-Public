#include "vpnofmes/ui/SettingsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

namespace vpnofmes::ui {

using vpnofmes::core::ConfigManager;

SettingsPage::SettingsPage(ConfigManager& configManager, QWidget* parent)
    : QWidget(parent), m_configManager(configManager) {
    buildUi();
    refreshFromConfig();
}

void SettingsPage::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 28, 32, 28);
    rootLayout->setSpacing(20);

    auto* title = new QLabel(QStringLiteral("Settings"), this);
    title->setObjectName("pageTitle");
    rootLayout->addWidget(title);

    // --- General behaviour card -----------------------------------------
    auto* generalCard = new QFrame(this);
    generalCard->setObjectName("card");
    auto* generalLayout = new QVBoxLayout(generalCard);
    generalLayout->setSpacing(10);

    auto* generalTitle = new QLabel(QStringLiteral("Application"), generalCard);
    generalTitle->setObjectName("cardTitle");
    generalLayout->addWidget(generalTitle);
    m_startMinimizedCheckbox = new QCheckBox(QStringLiteral("Start minimized to tray"), generalCard);
    m_minimizeToTrayCheckbox = new QCheckBox(QStringLiteral("Minimize to tray when the window is closed"), generalCard);
    generalLayout->addWidget(m_startMinimizedCheckbox);
    generalLayout->addWidget(m_minimizeToTrayCheckbox);
    rootLayout->addWidget(generalCard);

    // --- Reconnection card ------------------------------------------------
    auto* reconnectCard = new QFrame(this);
    reconnectCard->setObjectName("card");
    auto* reconnectLayout = new QVBoxLayout(reconnectCard);
    reconnectLayout->setSpacing(10);

    auto* reconnectTitle = new QLabel(QStringLiteral("Automatic reconnection"), reconnectCard);
    reconnectTitle->setObjectName("cardTitle");
    reconnectLayout->addWidget(reconnectTitle);

    m_autoReconnectCheckbox = new QCheckBox(QStringLiteral("Automatically reconnect if the connection drops"), reconnectCard);
    reconnectLayout->addWidget(m_autoReconnectCheckbox);

    auto* intervalRow = new QHBoxLayout();
    intervalRow->addWidget(new QLabel(QStringLiteral("Retry interval (seconds):"), reconnectCard));
    m_reconnectIntervalSpin = new QSpinBox(reconnectCard);
    m_reconnectIntervalSpin->setRange(1, 300);
    intervalRow->addWidget(m_reconnectIntervalSpin);
    intervalRow->addStretch();
    reconnectLayout->addLayout(intervalRow);

    auto* attemptsRow = new QHBoxLayout();
    attemptsRow->addWidget(new QLabel(QStringLiteral("Maximum attempts:"), reconnectCard));
    m_maxAttemptsSpin = new QSpinBox(reconnectCard);
    m_maxAttemptsSpin->setRange(1, 100);
    attemptsRow->addWidget(m_maxAttemptsSpin);
    attemptsRow->addStretch();
    reconnectLayout->addLayout(attemptsRow);

    rootLayout->addWidget(reconnectCard);

    // --- Updates card -------------------------------------------------
    auto* updatesCard = new QFrame(this);
    updatesCard->setObjectName("card");
    auto* updatesLayout = new QVBoxLayout(updatesCard);
    updatesLayout->setSpacing(10);

    auto* updatesTitle = new QLabel(QStringLiteral("Updates"), updatesCard);
    updatesTitle->setObjectName("cardTitle");
    updatesLayout->addWidget(updatesTitle);

    m_checkUpdatesOnStartupCheckbox = new QCheckBox(QStringLiteral("Check for updates on startup"), updatesCard);
    updatesLayout->addWidget(m_checkUpdatesOnStartupCheckbox);

    auto* updateRow = new QHBoxLayout();
    m_checkNowButton = new QPushButton(QStringLiteral("Check now"), updatesCard);
    m_checkNowButton->setObjectName("secondaryButton");
    m_updateStatusLabel = new QLabel(QStringLiteral("Up to date"), updatesCard);
    updateRow->addWidget(m_checkNowButton);
    updateRow->addWidget(m_updateStatusLabel);
    updateRow->addStretch();
    updatesLayout->addLayout(updateRow);

    rootLayout->addWidget(updatesCard);

    m_saveButton = new QPushButton(QStringLiteral("Save settings"), this);
    m_saveButton->setObjectName("primaryButton");
    rootLayout->addWidget(m_saveButton, 0, Qt::AlignLeft);
    rootLayout->addStretch();

    connect(m_saveButton, &QPushButton::clicked, this, &SettingsPage::applyToConfigAndSave);
    connect(m_checkNowButton, &QPushButton::clicked, this, &SettingsPage::checkForUpdatesRequested);
}

void SettingsPage::refreshFromConfig() {
    const auto& settings = m_configManager.settings();
    m_startMinimizedCheckbox->setChecked(settings.startMinimizedToTray);
    m_minimizeToTrayCheckbox->setChecked(settings.minimizeToTrayOnClose);
    m_autoReconnectCheckbox->setChecked(settings.autoReconnect);
    m_reconnectIntervalSpin->setValue(settings.reconnectIntervalSeconds);
    m_maxAttemptsSpin->setValue(settings.maxReconnectAttempts);
    m_checkUpdatesOnStartupCheckbox->setChecked(settings.checkForUpdatesOnStartup);
}

void SettingsPage::applyToConfigAndSave() {
    auto& settings = m_configManager.settings();
    settings.startMinimizedToTray = m_startMinimizedCheckbox->isChecked();
    settings.minimizeToTrayOnClose = m_minimizeToTrayCheckbox->isChecked();
    settings.autoReconnect = m_autoReconnectCheckbox->isChecked();
    settings.reconnectIntervalSeconds = m_reconnectIntervalSpin->value();
    settings.maxReconnectAttempts = m_maxAttemptsSpin->value();
    settings.checkForUpdatesOnStartup = m_checkUpdatesOnStartupCheckbox->isChecked();

    m_configManager.save();
    m_updateStatusLabel->setText(QStringLiteral("Settings saved"));
}

void SettingsPage::setUpdateStatusText(const QString& text) {
    m_updateStatusLabel->setText(text);
}

} // namespace vpnofmes::ui
