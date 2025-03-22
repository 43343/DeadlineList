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

class ChangePasswordForm : public QDialog
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

    SocketClient* m_socket;

};

#endif // CHANGEPASSWORDFORM_H
