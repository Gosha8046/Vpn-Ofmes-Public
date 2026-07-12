#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

#include "vpnofmes/core/ConfigManager.h"

namespace vpnofmes::ui {

/// Settings screen: tray behaviour, auto-reconnect policy and update
/// checking. Reads from / writes to the shared core::ConfigManager.
class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(vpnofmes::core::ConfigManager& configManager, QWidget* parent = nullptr);

    /// Copies the current in-memory settings into the form fields.
    void refreshFromConfig();

    void setUpdateStatusText(const QString& text);

signals:
    void checkForUpdatesRequested();

private:
    void buildUi();
    void applyToConfigAndSave();

    vpnofmes::core::ConfigManager& m_configManager;

    QCheckBox* m_startMinimizedCheckbox = nullptr;
    QCheckBox* m_minimizeToTrayCheckbox = nullptr;
    QCheckBox* m_autoReconnectCheckbox = nullptr;
    QSpinBox* m_reconnectIntervalSpin = nullptr;
    QSpinBox* m_maxAttemptsSpin = nullptr;
    QCheckBox* m_checkUpdatesOnStartupCheckbox = nullptr;
    QLabel* m_updateStatusLabel = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_checkNowButton = nullptr;
};

} // namespace vpnofmes::ui
