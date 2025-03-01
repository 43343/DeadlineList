#include "edittask.h"
#include "ui_edittask.h"

EditTask::EditTask(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditTask)
{
    ui->setupUi(this);
    ui->deadlineTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(1));
    ui->errorEdit->hide();

    connect(ui->acceptButton, &QPushButton::clicked, this, &EditTask::accepted);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}
void EditTask::accepted() {
    if(ui->lineEdit->text().isEmpty())
    {
        ui->lineEdit->setStyleSheet("border: 1px solid red;");
        ui->errorEdit->show();
    }
    else
    {
        accept();
    }
}

void EditTask::setTask(const QString& task)
{
    ui->lineEdit->setText(task);
}
void EditTask::setButtonAcceptText(const QString& buttonAcceptText)
{
    ui->acceptButton->setText(buttonAcceptText);
}
void EditTask::setDeadlineTime(const QDateTime& deadlineTime)
{
    ui->deadlineTimeEdit->setDateTime(deadlineTime);
}

QString EditTask::getTask() const {
    return ui->lineEdit->text();
}

QString EditTask::getDeadlineTime() const {
    return ui->deadlineTimeEdit->text();
}
QDateTime EditTask::getDeadlineDateTime() const {
    return ui->deadlineTimeEdit->dateTime();
}
EditTask::~EditTask()
{
    delete ui;
}
