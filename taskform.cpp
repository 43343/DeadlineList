#include "taskform.h"
#include "ui_taskform.h"
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include "edittask.h"

TaskForm::TaskForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TaskForm)
{
    ui->setupUi(this);

    connect(ui->edit, &QPushButton::clicked, this, &TaskForm::editTaskForm);
    connect(ui->remove, &QPushButton::clicked, this, &TaskForm::removeTaskForm);
    connect(ui->done, &QCheckBox::clicked, this, [this] () {
        emit changeDoneTask();
    });
}

TaskForm::~TaskForm()
{
    delete ui;
}

void TaskForm::setStyleSheetForWidget(const QString& style)
{
    ui->widget->setStyleSheet(style);
}

bool TaskForm::isDone() const
{
    return ui->done->isChecked();
}
QDateTime TaskForm::getDeadlineDateTime() const
{
    return deadlineDateTime;
}
QString TaskForm::getTask() const
{
    return ui->task->text();
}
QString TaskForm::getDeadline() const
{
    return ui->deadline->text();
}
void TaskForm::setDone(const bool& done)
{
    ui->done->setChecked(done);
}
void TaskForm::setDeadlineDateTime(const QDateTime& deadline)
{
    deadlineDateTime = deadline;
}
void TaskForm::setTask(const QString& task)
{
    ui->task->setText(task);
}
void TaskForm::setDeadline(const QString& deadline)
{
    ui->deadline->setText(deadline);
}

void TaskForm::editTaskForm()
{
    EditTask editTask(this);
    QString task = getTask();
    QString dateTime = getDeadline();
    editTask.setTask(task);
    editTask.setDeadlineTime(deadlineDateTime);
    switch (editTask.exec()) {
    case QDialog::Accepted:
        qDebug() << "Accepted";
        this->setParameters(editTask.getTask(), editTask.getDeadlineTime(), editTask.getDeadlineDateTime());
        emit taskEdited(this);
        break;
    case QDialog::Rejected:
        qDebug() << "Rejected";
        break;
    default:
        qDebug() << "Unexpected";
    }
}


void TaskForm::removeTaskForm()
{
    ui->edit->setDisabled(true);
    ui->done->setDisabled(true);
    ui->remove->setDisabled(true);
    QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(opacityEffect);

    QPropertyAnimation* fadeOutAnimation = new QPropertyAnimation(opacityEffect, "opacity");
    fadeOutAnimation->setDuration(500);
    fadeOutAnimation->setStartValue(1.0);
    fadeOutAnimation->setEndValue(0.0);

    QParallelAnimationGroup* animationGroup = new QParallelAnimationGroup(this);
    animationGroup->addAnimation(fadeOutAnimation);


    connect(animationGroup, &QParallelAnimationGroup::finished, this, [this]()
            {
                emit taskDeleted(this);
                this->deleteLater();
            });

    animationGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

void TaskForm::setParameters(const QString &taskEdit, const QString &dateTimeEdit, const QDateTime &dateTime)
{
    ui->task->setText(taskEdit);
    ui->deadline->setText(dateTimeEdit);
    deadlineDateTime = dateTime;
}
