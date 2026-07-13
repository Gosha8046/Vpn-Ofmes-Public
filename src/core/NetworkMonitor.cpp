#include "vpnofmes/core/NetworkMonitor.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <iphlpapi.h>
// GetIfTable2()/MIB_IF_TABLE2 (the "v2" IP Helper API) live in netioapi.h;
// iphlpapi.h does not reliably pull it in on its own, which previously
// produced "undeclared identifier" errors for both symbols.
#include <netioapi.h>
#elif defined(Q_OS_LINUX)
#include <QFile>
#include <QTextStream>
#endif

namespace vpnofmes::core {

NetworkMonitor::NetworkMonitor(QObject* parent) : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &NetworkMonitor::poll);
}

void NetworkMonitor::start(int intervalMs) {
    m_hasBaseline = false;
    m_elapsed.start();
    m_timer.start(intervalMs);
}

void NetworkMonitor::stop() {
    m_timer.stop();
    m_hasBaseline = false;
}

std::pair<quint64, quint64> NetworkMonitor::readCounters() {
    quint64 rx = 0;
    quint64 tx = 0;

#ifdef Q_OS_WIN
    MIB_IF_TABLE2* table = nullptr;
    if (GetIfTable2(&table) == NO_ERROR && table != nullptr) {
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            const MIB_IF_ROW2& row = table->Table[i];
            // Skip loopback and administratively-down interfaces.
            if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK || !row.InterfaceAndOperStatusFlags.HardwareInterface) {
                continue;
            }
            if (row.OperStatus != IfOperStatusUp) {
                continue;
            }
            rx += row.InOctets;
            tx += row.OutOctets;
        }
        FreeMibTable(table);
    }
#elif defined(Q_OS_LINUX)
    QFile file(QStringLiteral("/proc/net/dev"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        // First two lines are headers.
        stream.readLine();
        stream.readLine();
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            const int colon = line.indexOf(':');
            if (colon < 0) {
                continue;
            }
            const QString ifaceName = line.left(colon).trimmed();
            if (ifaceName == QStringLiteral("lo")) {
                continue;
            }
            const QStringList fields = line.mid(colon + 1).simplified().split(' ', Qt::SkipEmptyParts);
            if (fields.size() < 9) {
                continue;
            }
            rx += fields.at(0).toULongLong();
            tx += fields.at(8).toULongLong();
        }
    }
#else
    // Unsupported platform for live counters; the UI will simply show 0 KB/s.
#endif

    return {rx, tx};
}

void NetworkMonitor::poll() {
    const auto [rx, tx] = readCounters();
    const qint64 elapsedMs = m_elapsed.restart();

    if (!m_hasBaseline || elapsedMs <= 0) {
        m_lastRxBytes = rx;
        m_lastTxBytes = tx;
        m_hasBaseline = true;
        emit speedUpdated(0.0, 0.0);
        return;
    }

    const double seconds = elapsedMs / 1000.0;
    const double downloadKBps = rx >= m_lastRxBytes ? (rx - m_lastRxBytes) / 1024.0 / seconds : 0.0;
    const double uploadKBps = tx >= m_lastTxBytes ? (tx - m_lastTxBytes) / 1024.0 / seconds : 0.0;

    m_lastRxBytes = rx;
    m_lastTxBytes = tx;

    emit speedUpdated(downloadKBps, uploadKBps);
}

} // namespace vpnofmes::core
