#ifndef REGISTRATIONFORM_H
#define REGISTRATIONFORM_H

#include <QDialog>
#include <QString>
#include <QCloseEvent>
#include "socketclient.h"

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
    void registrationButtonClick();
    void errorRegistration(const QString& error);
    void successfullyRegistration();
    void back();

private:
    bool checkMinLength(const QString &password) const;
    bool containsUppercase(const QString &password) const;
    bool containsLowercase(const QString &password) const;
    bool containsDigit(const QString &password) const;
    bool containsSpecialChar(const QString &password) const;
    bool noSpacesAndCyrillic(const QString &password) const;
    bool validEmail(const QString &email) const;

    SocketClient* mSocket;

};

#endif // REGISTRATIONFORM_H
