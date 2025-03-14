#ifndef OTPINPUTWIDGET_H
#define OTPINPUTWIDGET_H
#include <QWidget>
#include <QLineEdit>
#include <QEvent>

class OTPInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OTPInputWidget(QWidget *parent = nullptr, int codeLength = 6);
    QString code() const;
    void applyErrorStyle();
    void applyErrorStyleAll();
    void resetStyle();
    void setEnabled(const bool& enabled);
private slots:
    void handleTextChanged(const QString text);
private:
    int m_codeLength;
    QList<QLineEdit*> m_fields;
    bool eventFilter(QObject *watched, QEvent *event);
};

#endif // OTPINPUTWIDGET_H
