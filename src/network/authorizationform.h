#ifndef AUTHORIZATIONFORM_H
#define AUTHORIZATIONFORM_H

#include <QWidget>
#include <QDialog>
#include <QEvent>
#include "socketclient.h"
#include "passwordvalidator.h"

namespace Ui {
class AuthorizationForm;
}

class AuthorizationForm : public QDialog
{
    Q_OBJECT

public:
    explicit AuthorizationForm(SocketClient* socket, QWidget *parent = nullptr);
    ~AuthorizationForm();


public slots:
    void loginButtonClick();
    void errorAuthorization(const QString& error);
    void successfullyAuthorization();
    void back();
    void forgetPassword();
    void continueForgetPassword();
    void successfullySendCodeEmail();
    void errorSendCodeEmail(const QString& error);
    void continueEnterCode();
    void backEnterCode();
    void successfullyContinueEnterCode();
    void errorContinueEnterCode(const QString& error);
    void confirmNewPassword();
    void successfullyConfirmNewPassword();
    void errorConfirmNewPassword(const QString& error);
    void validatePassword();

private:
    Ui::AuthorizationForm *ui;
    SocketClient* m_socket;
};

#endif // AUTHORIZATIONFORM_H
