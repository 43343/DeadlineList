#include "authorizationform.h"
#include "ui_authorizationform.h"

AuthorizationForm::AuthorizationForm(SocketClient* socket, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AuthorizationForm)
    , m_socket(socket)
{
    ui->setupUi(this);
    ui->passwordEdit->setEchoMode(QLineEdit::Password);
    ui->forgetPasswordButton->setCursor(Qt::PointingHandCursor);
    ui->resetNewPasswordEdit->setEchoMode(QLineEdit::Password);
    ui->confirmResetNewPasswordEdit->setEchoMode(QLineEdit::Password);
    ui->errorEmailOrPassword->hide();
    ui->emailNotFound->hide();
    ui->errorPassword->hide();
    ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }");
    connect(ui->resetNewPasswordEdit, &QLineEdit::textEdited , this, &AuthorizationForm::validatePassword);
    connect(ui->loginButton, &QPushButton::clicked, this, &AuthorizationForm::loginButtonClick);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->cancelButtonLabelWidget, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->closeButtonLabelWidget, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->backButtonLabelWidget, &QPushButton::clicked, this, &AuthorizationForm::back);
    connect(ui->forgetPasswordButton, &QPushButton::clicked, this, &AuthorizationForm::forgetPassword);
    connect(ui->backForgetPasswordButton, &QPushButton::clicked, this, &AuthorizationForm::back);
    connect(ui->continueForgetPasswordButton, &QPushButton::clicked, this, &AuthorizationForm::continueForgetPassword);
    connect(ui->continueButtonEnterCode, &QPushButton::clicked, this, &AuthorizationForm::continueEnterCode);
    connect(ui->backButtonEnterCode, &QPushButton::clicked, this, &AuthorizationForm::backEnterCode);
    connect(ui->confirmResetNewPasswordButton, &QPushButton::clicked, this, &AuthorizationForm::confirmNewPassword);
    connect(ui->backResetNewPasswordButton, &QPushButton::clicked, this, &AuthorizationForm::backEnterCode);
    connect(socket, &SocketClient::authorizationSuccessfully, this, &AuthorizationForm::successfullyAuthorization);
    connect(socket, &SocketClient::authorizationError, this, &AuthorizationForm::errorAuthorization);
    connect(socket, &SocketClient::sendCodeEmailResetPasswordSuccessfully, this, &AuthorizationForm::successfullySendCodeEmail);
    connect(socket, &SocketClient::sendCodeEmailResetPasswordError, this, &AuthorizationForm::errorSendCodeEmail);
    connect(socket, &SocketClient::checkCodeEmailResetPasswordSuccessfully, this, &AuthorizationForm::successfullyContinueEnterCode);
    connect(socket, &SocketClient::checkCodeEmailResetPasswordError, this, &AuthorizationForm::errorContinueEnterCode);
    connect(socket, &SocketClient::confirmResetNewPasswordSuccessfully, this, &AuthorizationForm::successfullyConfirmNewPassword);
    connect(socket, &SocketClient::confirmResetNewPasswordError, this, &AuthorizationForm::errorConfirmNewPassword);
}
void AuthorizationForm::loginButtonClick()
{
    ui->emailEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\";}");
    ui->errorEmailOrPassword->hide();
    ui->emailEdit->setEnabled(false);
    ui->passwordEdit->setEnabled(false);
    ui->loginButton->setEnabled(false);
    ui->forgetPasswordButton->setEnabled(false);
    ui->loginButton->setText("");
    ui->loginButton->startSpinner();
    m_socket->authorizationUser(ui->emailEdit->text(), ui->passwordEdit->text());
}
void AuthorizationForm::successfullyAuthorization()
{
    ui->emailEdit->setEnabled(true);
    ui->passwordEdit->setEnabled(true);
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Войти");
    ui->loginButton->stopSpinner();
    ui->closeButtonLabelWidget->show();
    ui->cancelButtonLabelWidget->hide();
    ui->backButtonLabelWidget->hide();
    ui->stackedWidget->setCurrentIndex(2);
    setMinimumSize(480,130);
    setMaximumSize(480,130);
    ui->labelWidgetText->setText("Авторизация прошла успешно.");
}
void AuthorizationForm::errorAuthorization(const QString& error)
{
    if(error == "User with this email and password not found.")
    {
        ui->emailEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\"; border: 1px solid red;}");
        ui->errorEmailOrPassword->show();
    }
    else
    {
        ui->closeButtonLabelWidget->hide();
        ui->cancelButtonLabelWidget->show();
        ui->backButtonLabelWidget->show();
        ui->stackedWidget->setCurrentIndex(2);
        setMinimumSize(480,130);
        setMaximumSize(480,130);
        ui->labelWidgetText->setText(error);
    }
    ui->emailEdit->setEnabled(true);
    ui->passwordEdit->setEnabled(true);
    ui->forgetPasswordButton->setEnabled(true);
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Войти");
    ui->loginButton->stopSpinner();
}
void AuthorizationForm::back()
{
    ui->stackedWidget->setCurrentIndex(0);
    setMinimumSize(392,229);
    setMaximumSize(392,229);
}
void AuthorizationForm::forgetPassword()
{
    ui->stackedWidget->setCurrentIndex(1);
    setMinimumSize(392,118);
    setMaximumSize(392,118);
}
void AuthorizationForm::continueForgetPassword()
{
    ui->emailEditForgetPassword->setStyleSheet("QLineEdit { font: 10pt \"Sitka\";}");
    ui->emailEditForgetPassword->setEnabled(false);
    ui->continueForgetPasswordButton->setEnabled(false);
    ui->backForgetPasswordButton->setEnabled(false);
    ui->continueForgetPasswordButton->setText("");
    ui->continueForgetPasswordButton->startSpinner();
    m_socket->sendCodeEmailResetPassword(ui->emailEditForgetPassword->text());
}
void AuthorizationForm::backEnterCode()
{
    ui->stackedWidget->setCurrentIndex(1);
    setMinimumSize(392,118);
    setMaximumSize(392,118);
}

void AuthorizationForm::successfullySendCodeEmail()
{
    ui->emailEditForgetPassword->setEnabled(true);
    ui->continueForgetPasswordButton->setEnabled(true);
    ui->backForgetPasswordButton->setEnabled(true);
    ui->continueForgetPasswordButton->setText("Продолжить");
    ui->continueForgetPasswordButton->stopSpinner();
    ui->resetPasswordEmailLabel->setText("На почту " + ui->emailEditForgetPassword->text() +  " был выслан код, введите его в поле ниже:");
    ui->stackedWidget->setCurrentIndex(3);
    setMinimumSize(392,186);
    setMaximumSize(392,186);
}
void AuthorizationForm::errorSendCodeEmail(const QString& error)
{
    if(error == "The user was not found by mail.")
    {
        ui->emailEditForgetPassword->setStyleSheet("QLineEdit { font: 10pt \"Sitka\"; border: 1px solid red;}");
        ui->emailNotFound->show();
    }
    else
    {
        ui->closeButtonLabelWidget->hide();
        ui->cancelButtonLabelWidget->show();
        ui->backButtonLabelWidget->show();
        ui->stackedWidget->setCurrentIndex(2);
        setMinimumSize(480,130);
        setMaximumSize(480,130);
        ui->labelWidgetText->setText(error);
    }
    ui->emailEditForgetPassword->setEnabled(true);
    ui->continueForgetPasswordButton->setEnabled(true);
    ui->backForgetPasswordButton->setEnabled(true);
    ui->continueForgetPasswordButton->setText("Продолжить");
    ui->continueForgetPasswordButton->stopSpinner();
}
void AuthorizationForm::continueEnterCode()
{
    ui->codeLineEdit->resetStyle();
    if(ui->codeLineEdit->code().length() < 6)
    {
        ui->codeLineEdit->applyErrorStyle();
        return;
    }
    ui->codeLineEdit->setEnabled(false);
    ui->continueButtonEnterCode->setEnabled(false);
    ui->backButtonEnterCode->setEnabled(false);
    ui->continueButtonEnterCode->startSpinner();
    m_socket->checkCodeEmailResetPassword(ui->codeLineEdit->code());
}
void AuthorizationForm::successfullyContinueEnterCode()
{
    ui->codeLineEdit->resetStyle();
    ui->codeLineEdit->setEnabled(true);
    ui->continueButtonEnterCode->setEnabled(true);
    ui->backButtonEnterCode->setEnabled(true);
    ui->continueButtonEnterCode->stopSpinner();
    ui->continueButtonEnterCode->setText("Продолжить");
    ui->stackedWidget->setCurrentIndex(4);
    setMinimumSize(600,329);
    setMaximumSize(600,329);
}
void AuthorizationForm::errorContinueEnterCode(const QString& error)
{
    if(error == "Incorrect or expired reset code.")
    {
        ui->codeLineEdit->applyErrorStyleAll();
    }
    else
    {
        ui->closeButtonLabelWidget->hide();
        ui->cancelButtonLabelWidget->show();
        ui->backButtonLabelWidget->show();
        ui->stackedWidget->setCurrentIndex(2);
        setMinimumSize(480,130);
        setMaximumSize(480,130);
        ui->labelWidgetText->setText(error);
    }
    ui->codeLineEdit->setEnabled(true);
    ui->continueButtonEnterCode->setEnabled(true);
    ui->backButtonEnterCode->setEnabled(true);
    ui->continueButtonEnterCode->stopSpinner();
    ui->continueButtonEnterCode->setText("Продолжить");
}
void AuthorizationForm::confirmNewPassword()
{
    if(ui->resetNewPasswordEdit->text() != ui->confirmResetNewPasswordEdit->text())
    {
        ui->confirmResetNewPasswordEdit->setStyleSheet("QLineEdit { border: 1px solid red;}");
        ui->errorPassword->show();
        return;
    }
    ui->confirmResetNewPasswordButton->setEnabled(false);
    ui->resetNewPasswordEdit->setEnabled(false);
    ui->continueButtonEnterCode->setEnabled(false);
    ui->backResetNewPasswordButton->setEnabled(false);
    ui->confirmResetNewPasswordButton->setText("");
    ui->confirmResetNewPasswordButton->startSpinner();
    m_socket->confirmResetNewPassword(ui->resetNewPasswordEdit->text());
}
void AuthorizationForm::successfullyConfirmNewPassword()
{

    ui->resetNewPasswordEdit->setEnabled(true);
    ui->confirmResetNewPasswordButton->setEnabled(true);
    ui->backResetNewPasswordButton->setEnabled(true);
    ui->confirmResetNewPasswordButton->stopSpinner();
    ui->confirmResetNewPasswordButton->setText("Подтвердить");
    ui->closeButtonLabelWidget->show();
    ui->cancelButtonLabelWidget->hide();
    ui->backButtonLabelWidget->hide();
    ui->stackedWidget->setCurrentIndex(2);
    setMinimumSize(480,130);
    setMaximumSize(480,130);
    ui->labelWidgetText->setText("Пароль изменен успешно.");
}
void AuthorizationForm::errorConfirmNewPassword(const QString& error)
{
    ui->closeButtonLabelWidget->hide();
    ui->cancelButtonLabelWidget->show();
    ui->backButtonLabelWidget->show();
    ui->stackedWidget->setCurrentIndex(2);
    setMinimumSize(480,130);
    setMaximumSize(480,130);
    ui->labelWidgetText->setText(error);
    ui->resetNewPasswordEdit->setEnabled(true);
    ui->confirmResetNewPasswordButton->setEnabled(true);
    ui->backResetNewPasswordButton->setEnabled(true);
    ui->confirmResetNewPasswordButton->stopSpinner();
    ui->confirmResetNewPasswordButton->setText("Подтвердить");
}
void AuthorizationForm::validatePassword()
{
    const bool& isMinLength = checkMinLength(ui->resetNewPasswordEdit->text());
    const bool& isContainsUppercase = containsUppercase(ui->resetNewPasswordEdit->text());
    const bool& isContainsLowercase = containsLowercase(ui->resetNewPasswordEdit->text());
    const bool& isContainsDigit = containsDigit(ui->resetNewPasswordEdit->text());
    const bool& isContainsSpecialChar = containsSpecialChar(ui->resetNewPasswordEdit->text());
    const bool& isNoSpacesAndCyrillic = noSpacesAndCyrillic(ui->resetNewPasswordEdit->text());
    isMinLength ? ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsUppercase ? ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsLowercase ? ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsDigit ? ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsSpecialChar ? ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isNoSpacesAndCyrillic ? ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->confirmResetNewPasswordButton->setEnabled(isMinLength && isContainsUppercase && isContainsLowercase && isContainsDigit && isContainsSpecialChar && isNoSpacesAndCyrillic);
}

AuthorizationForm::~AuthorizationForm()
{
    delete ui;
}
