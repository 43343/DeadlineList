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
    ui->errorEmailOrPassword->hide();
    ui->emailNotFound->hide();
    connect(ui->loginButton, &QPushButton::clicked, this, &AuthorizationForm::loginButtonClick);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->cancelButtonLabelWidget, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->closeButtonLabelWidget, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->backButtonLabelWidget, &QPushButton::clicked, this, &AuthorizationForm::back);
    connect(ui->forgetPasswordButton, &QPushButton::clicked, this, &AuthorizationForm::forgetPassword);
    connect(ui->backForgetPasswordButton, &QPushButton::clicked, this, &AuthorizationForm::back);
    connect(socket, &SocketClient::authorizationSuccessfully, this, &AuthorizationForm::successfullyAuthorization);
    connect(socket, &SocketClient::authorizationError, this, &AuthorizationForm::errorAuthorization);
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

AuthorizationForm::~AuthorizationForm()
{
    delete ui;
}
