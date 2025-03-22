#ifndef TASKFORM_H
#define TASKFORM_H

#include <QWidget>
#include <QDateTime>
#include "deletedtaskdata.h"

namespace Ui {
class TaskForm;
}

class TaskForm : public QWidget
{
    Q_OBJECT

public:
    explicit TaskForm(QWidget *parent = nullptr);
    ~TaskForm();

    void setParameters(const QString &taskEdit, const QString &dateTimeEdit, const QDateTime &dateTime, const QDateTime &changedDateTime, const QString &taskId);
    enum SyncStatus { Synced, Modified, Deleted };
    SyncStatus syncStatus() const;
    void setSyncStatus(SyncStatus status);

private slots:
    void removeTaskForm();
    void editTaskForm();
signals:
    void taskDeleted(TaskForm *task, DeletedTaskData deletedTask);
    void taskEdited(TaskForm *task);

public:
    void setStyleSheetForWidget(const QString& style);
    bool isDone() const;
    QDateTime getDeadlineDateTime() const;
    QString getTask() const;
    QString getDeadline() const;
    QDateTime getChangedDateTime() const;
    QString getTaskId() const;
    void setDone(const bool& done);
    void setDeadlineDateTime(const QDateTime& deadline);
    void setTask(const QString& task);
    void setDeadline(const QString& deadline);
    void setChangedDateTime(const QDateTime& deadline);
    void setTaskId(const QString &taskId);

private:
    Ui::TaskForm *ui;
    QDateTime m_deadlineDateTime;
    QDateTime m_changedDateTime;
    QString m_taskId;
    SyncStatus m_syncStatus = Modified;
};

#endif // TASKFORM_H
