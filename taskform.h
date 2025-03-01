#ifndef TASKFORM_H
#define TASKFORM_H

#include <QWidget>
#include <QDateTime>

namespace Ui {
class TaskForm;
}

class TaskForm : public QWidget
{
    Q_OBJECT

public:
    explicit TaskForm(QWidget *parent = nullptr);
    ~TaskForm();

    void setParameters(const QString &taskEdit, const QString &dateTimeEdit, const QDateTime &dateTime);

private slots:
    void removeTaskForm();
    void editTaskForm();
signals:
    void taskDeleted(TaskForm *task);
    void taskEdited(TaskForm *task);
    void changeDoneTask();

public:
    void setStyleSheetForWidget(const QString& style);
    bool isDone() const;
    QDateTime getDeadlineDateTime() const;
    QString getTask() const;
    QString getDeadline() const;
    void setDone(const bool& done);
    void setDeadlineDateTime(const QDateTime& deadline);
    void setTask(const QString& task);
    void setDeadline(const QString& deadline);

private:
    Ui::TaskForm *ui;
    QDateTime deadlineDateTime;
};

#endif // TASKFORM_H
