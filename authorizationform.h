#ifndef AUTHORIZATIONFORM_H
#define AUTHORIZATIONFORM_H

#include <QWidget>
#include <QDialog>
#include "socketclient.h"

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


private:
    Ui::AuthorizationForm *ui;
    SocketClient* m_socket;
};

#endif // AUTHORIZATIONFORM_H
