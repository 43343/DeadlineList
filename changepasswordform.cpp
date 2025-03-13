#include "changepasswordform.h"
#include "ui_changepasswordform.h"
#include <QRegularExpression>

ChangePasswordForm::ChangePasswordForm(SocketClient* socket, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChangePasswordForm)
    , mSocket(socket)
{
    ui->setupUi(this);
    ui->labelWidget->hide();
    ui->oldPasswordEdit->setEchoMode(QLineEdit::Password);
    ui->newPasswordEdit->setEchoMode(QLineEdit::Password);
    ui->confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    ui->errorOldPassword->hide();
    ui->errorPassword->hide();
    ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }");
    connect(ui->newPasswordEdit, &QLineEdit::textChanged, this, &ChangePasswordForm::validatePassword);
    connect(ui->changePasswordButton, &QPushButton::clicked, this, &ChangePasswordForm::changePasswordButtonClick);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->cancelButtonLabelWidget, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->closeButtonLabelWidget, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->backButtonLabelWidget, &QPushButton::clicked, this, &ChangePasswordForm::back);
    connect(socket, &SocketClient::changePasswordSuccessfully, this, &ChangePasswordForm::successfullyChangePassword);
    connect(socket, &SocketClient::changePasswordError, this, &ChangePasswordForm::errorChangePassword);
}
bool ChangePasswordForm::checkMinLength(const QString &password) const {
    return password.length() >= 8;
}


bool ChangePasswordForm::containsUppercase(const QString &password) const {
    QRegularExpression re("[A-Z]");
    return re.match(password).hasMatch();
}


bool ChangePasswordForm::containsLowercase(const QString &password) const {
    QRegularExpression re("[a-z]");
    return re.match(password).hasMatch();
}


bool ChangePasswordForm::containsDigit(const QString &password) const {
    QRegularExpression re("[0-9]");
    return re.match(password).hasMatch();
}


bool ChangePasswordForm::containsSpecialChar(const QString &password) const {
    // Символ '*' экранируется обратным слэшем.
    QRegularExpression re("[!@#$%^\\*]");
    return re.match(password).hasMatch();
}


bool ChangePasswordForm::noSpacesAndCyrillic(const QString &password) const {
    QRegularExpression re("[а-яА-ЯёЁ]");
    return !re.match(password).hasMatch() && !password.contains(' ');
}

void ChangePasswordForm::validatePassword()
{
    const bool& isMinLength = checkMinLength(ui->newPasswordEdit->text());
    const bool& isContainsUppercase = containsUppercase(ui->newPasswordEdit->text());
    const bool& isContainsLowercase = containsLowercase(ui->newPasswordEdit->text());
    const bool& isContainsDigit = containsDigit(ui->newPasswordEdit->text());
    const bool& isContainsSpecialChar = containsSpecialChar(ui->newPasswordEdit->text());
    const bool& isNoSpacesAndCyrillic = noSpacesAndCyrillic(ui->newPasswordEdit->text());
    isMinLength ? ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->minimumLength->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsUppercase ? ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containCapitalLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsLowercase ? ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containLowercaseLetter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsDigit ? ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containDigit->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isContainsSpecialChar ? ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->containSpecialCharacter->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    isNoSpacesAndCyrillic ? ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:black }") : ui->noSpacesAndCyrillic->setStyleSheet("QLabel { font: 10pt \"Sitka\"; color:red }");
    ui->changePasswordButton->setEnabled(isMinLength && isContainsUppercase && isContainsLowercase && isContainsDigit && isContainsSpecialChar && isNoSpacesAndCyrillic);
}
void ChangePasswordForm::changePasswordButtonClick()
{

    ui->errorOldPassword->hide();
    ui->errorPassword->hide();
    ui->oldPasswordEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\";}");
    ui->confirmPasswordEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\";}");
    const bool isValidConfirmPassword = ui->newPasswordEdit->text() == ui->confirmPasswordEdit->text();
    if(!isValidConfirmPassword)
    {
        ui->confirmPasswordEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\";  border: 1px solid red;}");
        ui->errorPassword->show();
        return;
    }
    ui->oldPasswordEdit->setEnabled(false);
    ui->newPasswordEdit->setEnabled(false);
    ui->confirmPasswordEdit->setEnabled(false);
    ui->changePasswordButton->setEnabled(false);
    ui->changePasswordButton->setText("");
    ui->changePasswordButton->startSpinner();
    mSocket->changePasswordUser(ui->oldPasswordEdit->text(), ui->newPasswordEdit->text());
}
void ChangePasswordForm::successfullyChangePassword()
{
    ui->oldPasswordEdit->setEnabled(true);
    ui->newPasswordEdit->setEnabled(true);
    ui->confirmPasswordEdit->setEnabled(true);
    ui->changePasswordButton->setEnabled(true);
    ui->changePasswordButton->setText("Сменить пароль");
    ui->changePasswordButton->stopSpinner();
    ui->changePasswordWidget->hide();
    ui->closeButtonLabelWidget->show();
    ui->cancelButtonLabelWidget->hide();
    ui->backButtonLabelWidget->hide();
    ui->labelWidget->show();
    ui->labelWidgetText->setText("Пароль изменен успешно.");
}
void ChangePasswordForm::errorChangePassword(const QString& error)
{
    if(error == "The old password was entered incorrectly.")
    {
        ui->oldPasswordEdit->setStyleSheet("QLineEdit { font: 10pt \"Sitka\"; border: 1px solid red;}");
        ui->errorOldPassword->show();
    }
    else
    {
        ui->changePasswordWidget->show();
        ui->closeButtonLabelWidget->hide();
        ui->cancelButtonLabelWidget->show();
        ui->backButtonLabelWidget->show();
        ui->labelWidget->show();
        ui->labelWidgetText->setText(error);
    }
    ui->oldPasswordEdit->setEnabled(true);
    ui->newPasswordEdit->setEnabled(true);
    ui->confirmPasswordEdit->setEnabled(true);
    ui->cancelButton->setEnabled(true);
    ui->changePasswordButton->setEnabled(true);
    ui->changePasswordButton->setText("Сменить пароль");
    ui->changePasswordButton->stopSpinner();
}
void ChangePasswordForm::back()
{
    ui->labelWidget->hide();
    ui->changePasswordWidget->show();
}

ChangePasswordForm::~ChangePasswordForm()
{
    delete ui;
}
