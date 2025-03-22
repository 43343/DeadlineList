#ifndef DELETEDTASKDATA_H
#define DELETEDTASKDATA_H
#include <QString>
#include <QDateTime>

struct DeletedTaskData
{
    QString taskId;
    QDateTime timeDeleted;
};

#endif // DELETEDTASKDATA_H
