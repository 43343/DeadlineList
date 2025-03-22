#include "spinnerbutton.h"

SpinnerButton::SpinnerButton(QWidget *parent)
    : QPushButton(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    connect(m_timer, &QTimer::timeout, this, [this](){
        setAngle((m_angle + 10) % 360);
    });
}

int SpinnerButton::angle() const
{
    return m_angle;
}

void SpinnerButton::setAngle(int angle)
{
    m_angle = angle;
    update();
}


void SpinnerButton::startSpinner()
{
    m_angle = 0;
    m_timer->start();
}

void SpinnerButton::stopSpinner()
{
    m_timer->stop();
    update();
}

void SpinnerButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);


    if (m_timer->isActive()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);


        int side = qMin(width(), height());
        int spinnerSize = side / 3;
        QRect spinnerRect((width() - spinnerSize) / 2,
                          (height() - spinnerSize) / 2,
                          spinnerSize,
                          spinnerSize);
        QPoint center = spinnerRect.center();


        painter.translate(center);
        painter.rotate(m_angle);
        painter.translate(-center);


        const int dotCount = 8;
        const double dotRadius = spinnerSize * 0.2;


        const double radiusMultiplier = 0.8;
        const double radius = spinnerSize * radiusMultiplier;


        for (int i = 0; i < dotCount; ++i) {
            double angleDeg = i * (360.0 / dotCount);
            double angleRad = qDegreesToRadians(angleDeg);
            int x = center.x() + static_cast<int>(radius * qCos(angleRad)) - dotRadius;
            int y = center.y() + static_cast<int>(radius * qSin(angleRad)) - dotRadius;

            int alpha = 255 - (i * (255 / dotCount));
            QColor color(0, 0, 0, alpha);

            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawEllipse(QPointF(x + dotRadius, y + dotRadius), dotRadius, dotRadius);
        }
    }
}
