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
        m_changedDateTime = QDateTime::currentDateTime();
        setSyncStatus(TaskForm::Modified);
        emit taskEdited(this);
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
    return m_deadlineDateTime;
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
    setSyncStatus(TaskForm::Modified);
    ui->done->setChecked(done);
}
void TaskForm::setDeadlineDateTime(const QDateTime& deadline)
{
    setSyncStatus(TaskForm::Modified);
    m_deadlineDateTime = deadline;
}
void TaskForm::setTask(const QString& task)
{
    setSyncStatus(TaskForm::Modified);
    ui->task->setText(task);
}
void TaskForm::setDeadline(const QString& deadline)
{
    setSyncStatus(TaskForm::Modified);
    ui->deadline->setText(deadline);
}

void TaskForm::editTaskForm()
{
    EditTask editTask(this);
    QString task = getTask();
    QString dateTime = getDeadline();
    editTask.setTask(task);
    editTask.setDeadlineTime(m_deadlineDateTime);
    switch (editTask.exec()) {
    case QDialog::Accepted:
        qDebug() << "Accepted";
        this->setParameters(editTask.getTask(), editTask.getDeadlineTime(), editTask.getDeadlineDateTime(), QDateTime::currentDateTime(), m_taskId);
        emit taskEdited(this);
        break;
    case QDialog::Rejected:
        qDebug() << "Rejected";
        break;
    default:
        qDebug() << "Unexpected";
    }
}
void TaskForm::setSyncStatus(SyncStatus status) {
    if(m_syncStatus != status) {
        m_syncStatus = status;
        m_changedDateTime = QDateTime::currentDateTime();
    }
}
TaskForm::SyncStatus TaskForm::syncStatus() const {

    return  m_syncStatus;
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
                setSyncStatus(TaskForm::Deleted);
                DeletedTaskData deletedTask;
                deletedTask.taskId = m_taskId;
                deletedTask.timeDeleted = QDateTime::currentDateTime();
                emit taskDeleted(this, deletedTask);
                this->deleteLater();
            });

    animationGroup->start(QAbstractAnimation::DeleteWhenStopped);
}
void TaskForm::setChangedDateTime(const QDateTime& changedDateTime)
{
    m_changedDateTime = changedDateTime;
    setSyncStatus(TaskForm::Modified);
}
QDateTime TaskForm::getChangedDateTime() const
{
    return m_changedDateTime;
}

QString TaskForm::getTaskId() const
{
    return m_taskId;
}
void TaskForm::setTaskId(const QString &taskId)
{
    m_taskId = taskId;
}

void TaskForm::setParameters(const QString &taskEdit, const QString &dateTimeEdit, const QDateTime &dateTime,  const QDateTime &changedDateTime, const QString &taskId)
{
    ui->task->setText(taskEdit);
    ui->deadline->setText(dateTimeEdit);
    m_deadlineDateTime = dateTime;
    m_changedDateTime = changedDateTime;
    m_taskId = taskId;
    setSyncStatus(TaskForm::Modified);
}
