#pragma once

#include <QWidget>
#include <QLabel>

namespace vpnofmes::ui {

/// Displays live download/upload throughput, fed by
/// core::NetworkMonitor::speedUpdated().
class SpeedWidget : public QWidget {
    Q_OBJECT

public:
    explicit SpeedWidget(QWidget* parent = nullptr);

public slots:
    /// speeds are expressed in kilobytes/second.
    void updateSpeeds(double downloadKBps, double uploadKBps);
    void reset();

private:
    static QString formatSpeed(double kbps);

    QLabel* m_downloadLabel = nullptr;
    QLabel* m_uploadLabel = nullptr;
};

} // namespace vpnofmes::ui
