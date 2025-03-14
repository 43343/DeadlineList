#ifndef CHANGEPASSWORDFORM_H
#define CHANGEPASSWORDFORM_H

#include <QDialog>
#include <QString>
#include <QCloseEvent>
#include "socketclient.h"
#include "passwordvalidator.h"

namespace Ui {
class ChangePasswordForm;
}

class ChangePasswordForm : public QDialog, PasswordValidator
{
    Q_OBJECT

public:
    explicit ChangePasswordForm(SocketClient* socket, QWidget *parent = nullptr);
    ~ChangePasswordForm();

private:
    Ui::ChangePasswordForm *ui;

private slots:
    void validatePassword();
    void changePasswordButtonClick();
    void errorChangePassword(const QString& error);
    void successfullyChangePassword();
    void back();

private:
    bool checkMinLength(const QString &password) const;
    bool containsUppercase(const QString &password) const;
    bool containsLowercase(const QString &password) const;
    bool containsDigit(const QString &password) const;
    bool containsSpecialChar(const QString &password) const;
    bool noSpacesAndCyrillic(const QString &password) const;

    SocketClient* mSocket;

};

#endif // CHANGEPASSWORDFORM_H
