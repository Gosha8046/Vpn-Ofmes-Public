#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QColor>

#include "vpnofmes/core/ConnectionManager.h"

namespace vpnofmes::ui {

/// Small round indicator with a soft pulsing glow used on the Home screen
/// to reflect the current ConnectionState at a glance:
///   - grey:   Disconnected
///   - amber, pulsing: Connecting / Reconnecting
///   - purple accent, steady glow: Connected
///   - red:    Error
class StatusIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal pulse READ pulse WRITE setPulse)

public:
    explicit StatusIndicator(QWidget* parent = nullptr);

    void setConnectionState(vpnofmes::core::ConnectionState state);

    [[nodiscard]] qreal pulse() const { return m_pulse; }
    void setPulse(qreal value);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void applyStateAnimation();

    vpnofmes::core::ConnectionState m_state = vpnofmes::core::ConnectionState::Disconnected;
    QColor m_color;
    qreal m_pulse = 1.0;
    QPropertyAnimation* m_animation = nullptr;
};

} // namespace vpnofmes::ui
