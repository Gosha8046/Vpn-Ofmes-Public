#include "vpnofmes/ui/SpeedWidget.h"

#include <QHBoxLayout>

namespace vpnofmes::ui {

SpeedWidget::SpeedWidget(QWidget* parent) : QWidget(parent) {
    m_downloadLabel = new QLabel(QStringLiteral("↓ 0.0 KB/s"), this);
    m_uploadLabel = new QLabel(QStringLiteral("↑ 0.0 KB/s"), this);

    m_downloadLabel->setObjectName("downloadSpeedLabel");
    m_uploadLabel->setObjectName("uploadSpeedLabel");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);
    layout->addWidget(m_downloadLabel);
    layout->addWidget(m_uploadLabel);
    layout->addStretch();
}

QString SpeedWidget::formatSpeed(double kbps) {
    if (kbps >= 1024.0) {
        return QStringLiteral("%1 MB/s").arg(kbps / 1024.0, 0, 'f', 2);
    }
    return QStringLiteral("%1 KB/s").arg(kbps, 0, 'f', 1);
}

void SpeedWidget::updateSpeeds(double downloadKBps, double uploadKBps) {
    m_downloadLabel->setText(QStringLiteral("↓ %1").arg(formatSpeed(downloadKBps)));
    m_uploadLabel->setText(QStringLiteral("↑ %1").arg(formatSpeed(uploadKBps)));
}

void SpeedWidget::reset() {
    updateSpeeds(0.0, 0.0);
}

} // namespace vpnofmes::ui
