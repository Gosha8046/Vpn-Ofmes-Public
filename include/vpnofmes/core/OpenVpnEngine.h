#pragma once

#include <QProcess>
#include <QTcpSocket>

#include "vpnofmes/core/VpnEngine.h"

namespace vpnofmes::core {

/// Drives the official OpenVPN community binary (openvpn.exe) the same
/// way OpenVPN-GUI does: the process is started with its documented
/// "management interface" enabled (--management 127.0.0.1 <port>), and
/// this class talks to that local TCP socket using OpenVPN's own
/// plain-text management protocol to observe state transitions and to
/// request a clean shutdown. No tunneling or cryptographic logic is
/// reimplemented; everything happens inside the OpenVPN process itself.
class OpenVpnEngine : public VpnEngine {
    Q_OBJECT

public:
    explicit OpenVpnEngine(QObject* parent = nullptr);

    [[nodiscard]] QString engineName() const override { return QStringLiteral("OpenVPN"); }
    [[nodiscard]] QString locateExecutable() const override;
    [[nodiscard]] EngineState state() const override { return m_state; }

    void connectToServer(const ServerProfile& profile,
                         const QString& login,
                         const QString& password) override;
    void disconnectFromServer() override;

private slots:
    void onManagementConnected();
    void onManagementReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void setState(EngineState newState);
    QString writeAuthFile(const QString& login, const QString& password);
    void cleanupAuthFile();

    EngineState m_state = EngineState::Idle;
    QProcess* m_process = nullptr;
    QTcpSocket* m_managementSocket = nullptr;
    quint16 m_managementPort = 0;
    QString m_authFilePath;
    bool m_userRequestedDisconnect = false;
};

} // namespace vpnofmes::core
