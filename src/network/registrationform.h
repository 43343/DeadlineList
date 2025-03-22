#ifndef REGISTRATIONFORM_H
#define REGISTRATIONFORM_H

#include <QDialog>
#include <QString>
#include <QCloseEvent>
#include "socketclient.h"
#include "passwordvalidator.h"

namespace Ui {
class RegistrationForm;
}

class RegistrationForm : public QDialog
{
    Q_OBJECT

public:
    explicit RegistrationForm(SocketClient* socket, QWidget *parent = nullptr);
    ~RegistrationForm();

private:
    Ui::RegistrationForm *ui;

private slots:
    void validatePassword();
    void sendCodeRegistrationButtonClick();
    void checkCodeRegistrationButtonClick();
    void errorSendCodeRegistration(const QString& error);
    void successfullySendCodeRegistration();
    void errorCheckCodeRegistration(const QString& error);
    void successfullyCheckCodeRegistration();
    void back();

private:
    bool validEmail(const QString &email) const;

    SocketClient* m_socket;

};

#endif // REGISTRATIONFORM_H
