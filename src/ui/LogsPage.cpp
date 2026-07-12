#include "vpnofmes/ui/LogsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>

namespace vpnofmes::ui {

using vpnofmes::core::LogLevel;
using vpnofmes::core::Logger;

LogsPage::LogsPage(QWidget* parent) : QWidget(parent) {
    buildUi();
}

void LogsPage::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 28, 32, 28);
    rootLayout->setSpacing(16);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel(QStringLiteral("Logs"), this);
    title->setObjectName("pageTitle");
    header->addWidget(title);
    header->addStretch();

    m_openFolderButton = new QPushButton(QStringLiteral("Open logs folder"), this);
    m_openFolderButton->setObjectName("secondaryButton");
    m_clearButton = new QPushButton(QStringLiteral("Clear view"), this);
    m_clearButton->setObjectName("secondaryButton");
    header->addWidget(m_openFolderButton);
    header->addWidget(m_clearButton);
    rootLayout->addLayout(header);

    m_logView = new QTextEdit(this);
    m_logView->setObjectName("logView");
    m_logView->setReadOnly(true);
    m_logView->setStyleSheet(QStringLiteral("font-family: Consolas, 'Courier New', monospace;"));
    rootLayout->addWidget(m_logView);

    connect(m_clearButton, &QPushButton::clicked, m_logView, &QTextEdit::clear);
    connect(m_openFolderButton, &QPushButton::clicked, this, [this]() {
        const QString path = Logger::instance().logFilePath();
        const QString dir = QFileInfo(path).absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
}

QString LogsPage::colorForLevel(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return QStringLiteral("#E6E6E6");
        case LogLevel::Warning: return QStringLiteral("#E0A128");
        case LogLevel::Error:   return QStringLiteral("#E03C3C");
        case LogLevel::Debug:   return QStringLiteral("#9B7BD6");
    }
    return QStringLiteral("#E6E6E6");
}

void LogsPage::appendEntry(const QString& formattedLine, LogLevel level) {
    const QString escaped = formattedLine.toHtmlEscaped();
    m_logView->append(QStringLiteral("<span style=\"color:%1;\">%2</span>")
                           .arg(colorForLevel(level), escaped));
}

void LogsPage::loadHistory(const QStringList& history) {
    for (const QString& line : history) {
        appendEntry(line, LogLevel::Info);
    }
}

} // namespace vpnofmes::ui
