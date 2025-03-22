#include "registrationform.h"
#include "ui_registrationform.h"
#include <QRegularExpression>

RegistrationForm::RegistrationForm(SocketClient* socket, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegistrationForm)
    , m_socket(socket)
{
    ui->setupUi(this);
    ui->passwordEdit->setEchoMode(QLineEdit::Password);
    ui->confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    ui->errorEmail->hide();
    ui->errorPassword->hide();
    ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }");
    ui->emailEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\";}");
    connect(ui->passwordEdit, &QLineEdit::textChanged, this, &RegistrationForm::validatePassword);
    connect(ui->registerButton, &QPushButton::clicked, this, &RegistrationForm::sendCodeRegistrationButtonClick);
    connect(ui->confirmationButtonEnterCode, &QPushButton::clicked, this, &RegistrationForm::checkCodeRegistrationButtonClick);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->cancelButtonLabelWidget, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->closeButtonLabelWidget, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->backButtonLabelWidget, &QPushButton::clicked, this, &RegistrationForm::back);
    connect(ui->backButtonEnterCode, &QPushButton::clicked, this, &RegistrationForm::back);
    connect(socket, &SocketClient::sendCodeRegistrationSuccessfully, this, &RegistrationForm::successfullySendCodeRegistration);
    connect(socket, &SocketClient::sendCodeRegistrationError, this, &RegistrationForm::errorSendCodeRegistration);
    connect(socket, &SocketClient::checkCodeRegistrationSuccessfully, this, &RegistrationForm::successfullyCheckCodeRegistration);
    connect(socket, &SocketClient::checkCodeRegistrationError, this, &RegistrationForm::errorCheckCodeRegistration);
}

void RegistrationForm::validatePassword()
{
    const bool& isMinLength = checkMinLength(ui->passwordEdit->text());
    const bool& isContainsUppercase = containsUppercase(ui->passwordEdit->text());
    const bool& isContainsLowercase = containsLowercase(ui->passwordEdit->text());
    const bool& isContainsDigit = containsDigit(ui->passwordEdit->text());
    const bool& isContainsSpecialChar = containsSpecialChar(ui->passwordEdit->text());
    const bool& isNoSpacesAndCyrillic = noSpacesAndCyrillic(ui->passwordEdit->text());
    isMinLength ? ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsUppercase ? ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsLowercase ? ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsDigit ? ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsSpecialChar ? ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isNoSpacesAndCyrillic ? ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->registerButton->setEnabled(isMinLength && isContainsUppercase && isContainsLowercase && isContainsDigit && isContainsSpecialChar && isNoSpacesAndCyrillic);
}
bool RegistrationForm::validEmail(const QString &email) const
{
    QRegularExpression emailRegex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+.[A-Za-z]{2,}$)");
    QRegularExpressionMatch match = emailRegex.match(email);
    return match.hasMatch();
}
void RegistrationForm::sendCodeRegistrationButtonClick()
{

    ui->errorEmail->hide();
    ui->errorPassword->hide();
    ui->emailEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\";}");
    ui->confirmPasswordEdit->setStyleSheet("QLineEdit { }");
    const bool isValidEmail = validEmail(ui->emailEdit->text());
    const bool isValidConfirmPassword = ui->passwordEdit->text() == ui->confirmPasswordEdit->text();
    if(!isValidEmail)
    {
        ui->emailEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\"; border: 1px solid red;}");
        ui->errorEmail->setText("                                                                            *Почта введена некорректно");
        ui->errorEmail->show();
    }
    if(!isValidConfirmPassword)
    {
        ui->confirmPasswordEdit->setStyleSheet("QLineEdit { border: 1px solid red;}");
        ui->errorPassword->show();
    }
    if(!isValidEmail || !isValidConfirmPassword)
        return;
    ui->emailEdit->setEnabled(false);
    ui->passwordEdit->setEnabled(false);
    ui->confirmPasswordEdit->setEnabled(false);
    ui->registerButton->setEnabled(false);
    ui->registerButton->setText("");
    ui->registerButton->startSpinner();
    m_socket->sendCodeRegisterUser(ui->emailEdit->text());
}
void RegistrationForm::checkCodeRegistrationButtonClick()
{
    ui->codeLineEdit->resetStyle();
    if(ui->codeLineEdit->code().length() < 6)
    {
        ui->codeLineEdit->applyErrorStyle();
        return;
    }
    ui->codeLineEdit->setEnabled(false);
    ui->confirmationButtonEnterCode->setEnabled(false);
    ui->backButtonEnterCode->setEnabled(false);
    ui->confirmationButtonEnterCode->setText("");
    ui->confirmationButtonEnterCode->startSpinner();
    m_socket->checkCodeRegisterUser(ui->emailEdit->text(), ui->passwordEdit->text(), ui->codeLineEdit->code());
}
void RegistrationForm::successfullySendCodeRegistration()
{
    ui->emailEdit->setEnabled(true);
    ui->passwordEdit->setEnabled(true);
    ui->confirmPasswordEdit->setEnabled(true);
    ui->registerButton->setEnabled(true);
    ui->registerButton->setText("Зарегистрироваться");
    ui->registerButton->stopSpinner();
    ui->closeButtonLabelWidget->show();
    ui->cancelButtonLabelWidget->hide();
    ui->backButtonLabelWidget->hide();
    ui->stackedWidget->setCurrentIndex(1);
    ui->confirmationEmailLabel->setText("На почту " + ui->emailEdit->text() +  " был выслан код, введите его в поле ниже:");
    setMinimumSize(392,186);
    setMaximumSize(392,186);
    ui->registrationLabel->setMinimumWidth(392);
    ui->registrationLabel->setMaximumWidth(392);
}
void RegistrationForm::errorSendCodeRegistration(const QString& error)
{
    if(error == "The user with this email already exists.")
    {
        ui->emailEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\"; border: 1px solid red;}");
        ui->errorEmail->setText("                                               *Пользователь с такой почтой уже существует");
        ui->errorEmail->show();
    }
    else
    {
        ui->closeButtonLabelWidget->hide();
        ui->cancelButtonLabelWidget->show();
        ui->backButtonLabelWidget->show();
        ui->stackedWidget->setCurrentIndex(2);
        ui->labelWidgetText->setText(error);
        setMinimumSize(480,130);
        setMaximumSize(480,130);
        ui->registrationLabel->setMinimumWidth(480);
        ui->registrationLabel->setMaximumWidth(480);
    }
    ui->emailEdit->setEnabled(true);
    ui->passwordEdit->setEnabled(true);
    ui->confirmPasswordEdit->setEnabled(true);
    ui->cancelButton->setEnabled(true);
    ui->registerButton->setEnabled(true);
    ui->registerButton->setText("Зарегистрироваться");
    ui->registerButton->stopSpinner();
}
void RegistrationForm::successfullyCheckCodeRegistration()
{
    ui->codeLineEdit->resetStyle();
    ui->codeLineEdit->setEnabled(true);
    ui->confirmationButtonEnterCode->setEnabled(true);
    ui->backButtonEnterCode->setEnabled(true);
    ui->confirmationButtonEnterCode->stopSpinner();
    ui->confirmationButtonEnterCode->setText("Подтвердить");
    ui->stackedWidget->setCurrentIndex(2);
    ui->labelWidgetText->setText("Регистрация прошла успешно.");
    setMinimumSize(480,130);
    setMaximumSize(480,130);
    ui->registrationLabel->setMinimumWidth(480);
    ui->registrationLabel->setMaximumWidth(480);
}
void RegistrationForm::errorCheckCodeRegistration(const QString& error)
{
    if(error == "Incorrect or expired confirmation code.")
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
    ui->confirmationButtonEnterCode->setEnabled(true);
    ui->backButtonEnterCode->setEnabled(true);
    ui->confirmationButtonEnterCode->stopSpinner();
    ui->confirmationButtonEnterCode->setText("Подтвердить");
}
void RegistrationForm::back()
{
    ui->stackedWidget->setCurrentIndex(0);
    setMinimumSize(600,380);
    setMaximumSize(600,380);
    ui->registrationLabel->setMinimumWidth(600);
    ui->registrationLabel->setMaximumWidth(600);
}

RegistrationForm::~RegistrationForm()
{
    delete ui;
}
