#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

namespace vpnofmes::core {

/// Periodically samples the network interface byte counters and turns the
/// deltas into a download/upload speed reading for the UI's speed
/// indicator.
///
/// The counters come straight from the operating system's own interface
/// statistics (the IP Helper API on Windows, /proc/net/dev on Linux for
/// development builds) -- no packet inspection or custom accounting is
/// performed.
class NetworkMonitor : public QObject {
    Q_OBJECT

public:
    explicit NetworkMonitor(QObject* parent = nullptr);

    /// Begins sampling on the given interval (milliseconds).
    void start(int intervalMs = 1000);
    void stop();

signals:
    /// Emitted on every sample with instantaneous speeds in kilobytes/second.
    void speedUpdated(double downloadKBps, double uploadKBps);

private:
    void poll();
    /// Returns total {receivedBytes, sentBytes} summed across active,
    /// non-loopback network interfaces.
    static std::pair<quint64, quint64> readCounters();

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    quint64 m_lastRxBytes = 0;
    quint64 m_lastTxBytes = 0;
    bool m_hasBaseline = false;
};

} // namespace vpnofmes::core
