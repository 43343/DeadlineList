#ifndef SPINNERBUTTON_H
#define SPINNERBUTTON_H
#include <QPushButton>
#include <QPainter>
#include <QTimer>

class SpinnerButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(int angle READ angle WRITE setAngle)
public:
    explicit SpinnerButton(QWidget *parent = nullptr);
    int angle() const;
    void setAngle(int angle);
    void startSpinner();
    void stopSpinner();
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    int m_angle = 0;
    QTimer *m_timer;
};

#endif // SPINNERBUTTON_H
