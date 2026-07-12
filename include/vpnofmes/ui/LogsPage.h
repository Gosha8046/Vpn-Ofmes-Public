#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

#include "vpnofmes/core/Logger.h"

namespace vpnofmes::ui {

/// Logs screen: a live, colour-coded view of everything written through
/// core::Logger since the application started.
class LogsPage : public QWidget {
    Q_OBJECT

public:
    explicit LogsPage(QWidget* parent = nullptr);

    /// Loads whatever has already been logged before this page existed.
    void loadHistory(const QStringList& history);

public slots:
    void appendEntry(const QString& formattedLine, vpnofmes::core::LogLevel level);

private:
    void buildUi();
    static QString colorForLevel(vpnofmes::core::LogLevel level);

    QTextEdit* m_logView = nullptr;
    QPushButton* m_clearButton = nullptr;
    QPushButton* m_openFolderButton = nullptr;
};

} // namespace vpnofmes::ui
