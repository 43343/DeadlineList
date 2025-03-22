#ifndef SHADOWBUTTON_H
#define SHADOWBUTTON_H
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QEnterEvent>

class ShadowButton : public QPushButton
{
public:
    ShadowButton(QWidget *parent);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

};

#endif // SHADOWBUTTON_H
