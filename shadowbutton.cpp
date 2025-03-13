#include "shadowbutton.h"

ShadowButton::ShadowButton(QWidget *parent) : QPushButton(parent) {}

void ShadowButton::enterEvent(QEnterEvent *event)
{
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(8);
    shadow->setOffset(0, 3);
    shadow->setColor(Qt::black);
    setGraphicsEffect(shadow);


    QPushButton::enterEvent(event);
}
void ShadowButton::leaveEvent(QEvent *event)
{
    setGraphicsEffect(nullptr);
    QPushButton::leaveEvent(event);
}
