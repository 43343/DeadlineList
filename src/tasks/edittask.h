#ifndef EDITTASK_H
#define EDITTASK_H

#include <QDialog>
#include <QDateTime>

namespace Ui {
class EditTask;
}

class EditTask : public QDialog
{
    Q_OBJECT

public:
    explicit EditTask( QWidget *parent = nullptr);
    ~EditTask();

private:
    Ui::EditTask *ui;

public:
    QString getTask() const;
    QString getDeadlineTime() const;
    QDateTime getDeadlineDateTime() const;
    void setTask(const QString& task);
    void setDeadlineTime(const QDateTime& deadlineTime);
    void setButtonAcceptText(const QString& buttonAcceptText);
private slots:
    void accepted();
};

#endif // EDITTASK_H
