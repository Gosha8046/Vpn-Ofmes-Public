// Vpn-Ofmes - entry point.
//
// Responsible only for standing up QApplication with the right identity
// (used by QStandardPaths to pick a per-user config directory) and
// handing control to MainWindow, which acts as the composition root for
// every other module.

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>

#include "vpnofmes/ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Vpn-Ofmes"));
    QApplication::setApplicationName(QStringLiteral("Vpn-Ofmes"));

    // The app lives on after its window is closed (minimize-to-tray), so
    // Qt must not quit just because the last visible window disappeared.
    QApplication::setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::warning(nullptr, QStringLiteral("Vpn-Ofmes"),
                              QStringLiteral("No system tray was detected. "
                                             "Minimize-to-tray will be unavailable."));
    }

    vpnofmes::ui::MainWindow window;

    return QApplication::exec();
}
