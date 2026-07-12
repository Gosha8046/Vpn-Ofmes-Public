#include "vpnofmes/ui/StatusIndicator.h"

#include <QPainter>
#include <QRadialGradient>

namespace vpnofmes::ui {

using vpnofmes::core::ConnectionState;

StatusIndicator::StatusIndicator(QWidget* parent) : QWidget(parent) {
    setFixedSize(18, 18);
    m_color = QColor("#8A8A8A"); // Disconnected grey by default.

    m_animation = new QPropertyAnimation(this, "pulse", this);
    m_animation->setStartValue(0.4);
    m_animation->setEndValue(1.0);
    m_animation->setDuration(900);
    m_animation->setEasingCurve(QEasingCurve::InOutSine);
}

void StatusIndicator::setPulse(qreal value) {
    m_pulse = value;
    update();
}

void StatusIndicator::setConnectionState(ConnectionState state) {
    if (m_state == state) {
        return;
    }
    m_state = state;
    applyStateAnimation();
}

void StatusIndicator::applyStateAnimation() {
    m_animation->stop();

    switch (m_state) {
        case ConnectionState::Disconnected:
            m_color = QColor("#8A8A8A");
            m_pulse = 1.0;
            update();
            break;
        case ConnectionState::Connecting:
        case ConnectionState::Reconnecting:
            m_color = QColor("#E0A128");
            m_animation->setLoopCount(-1); // pulse forever until state changes
            m_animation->start();
            break;
        case ConnectionState::Connected:
            m_color = QColor("#7B2EFF");
            m_pulse = 1.0;
            update();
            break;
        case ConnectionState::Error:
            m_color = QColor("#E03C3C");
            m_pulse = 1.0;
            update();
            break;
    }
}

void StatusIndicator::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(2, 2, -2, -2);
    const QPointF center = bounds.center();
    const qreal radius = bounds.width() / 2.0;

    // Soft outer glow, its intensity driven by the pulse animation.
    QRadialGradient glow(center, radius * 1.8);
    QColor glowColor = m_color;
    glowColor.setAlphaF(0.55 * m_pulse);
    glow.setColorAt(0.0, glowColor);
    glowColor.setAlphaF(0.0);
    glow.setColorAt(1.0, glowColor);
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(center, radius * 1.8, radius * 1.8);

    // Solid core dot.
    painter.setBrush(m_color);
    painter.drawEllipse(bounds);
}

} // namespace vpnofmes::ui
