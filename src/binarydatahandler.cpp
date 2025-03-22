#include "binarydatahandler.h"
#include <QFile>
#include <QDir>
#include <QDataStream>
#include <QDebug>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QByteArray>
#include <QDataStream>

QString fullFilePath(const QString &fileName)
{
#ifdef Q_OS_WIN
    QString baseFolder = QDir::homePath() + "/AppData/Roaming/DeadlineList";
#endif
#ifdef Q_OS_LINUX
    QString baseFolder = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/DeadlineList";
#endif
    QDir dir(baseFolder);
    if (!dir.exists()) {
        // Если папки не существует, пробуем её создать
        if (!dir.mkpath(baseFolder)) {
            qWarning() << "Не удалось создать каталог:" << baseFolder;
        }
    }
    return dir.absoluteFilePath(fileName);
}
bool deleteFile(const QString& fileName)
{
    QString m_filePath = fullFilePath(fileName);
    return QFile::remove(m_filePath);
}

bool overwritingFile(const QString &fileName, const Config* config)
{
    if (config == nullptr)
        return false;

    // Формируем полный путь к файлу и сохраняем его в члене класса
    QString m_filePath = fullFilePath(fileName);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Не удалось открыть файл для записи:" << m_filePath;
        return false;
    }

    QDataStream out(&file);

    // Запись данных. Порядок записи должен совпадать с порядком чтения.
    out << config->enableTextNotifications;
    out << config->enableSoundNotifications;
    out << config->launchByDefault;
    out << config->launchingTray;

    file.close();
    return true;
}

bool loadFromFile(const QString &fileName, Config* config)
{
    if (config == nullptr)
        return false;
    QString m_filePath = fullFilePath(fileName);

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл для чтения:" << m_filePath;
        return false;
    }

    QDataStream in(&file);

    in >> config->enableTextNotifications;
    in >> config->enableSoundNotifications;
    in >> config->launchByDefault;
    in >> config->launchingTray;

    file.close();
    return true;
}
bool overwritingFile(const QString &fileName, const QList<DeletedTaskData>* taskList)
{

    // Формируем полный путь к файлу и сохраняем его в члене класса
    QString m_filePath = fullFilePath(fileName);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Не удалось открыть файл для записи:" << m_filePath;
        return false;
    }

    QDataStream out(&file);

    out.setVersion(QDataStream::Qt_5_9);

    // Записываем количество задач
    out << static_cast<qint32>(taskList->size());

    for (const DeletedTaskData task : *taskList) {
        out << task.taskId;
        out << task.timeDeleted;
    }

    file.close();
    return true;
}

bool loadFromFile(const QString &fileName, QList<DeletedTaskData>* taskList)
{
    QString m_filePath = fullFilePath(fileName);

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл для чтения:" << m_filePath;
        return false;
    }

    QDataStream in(&file);

    in.setVersion(QDataStream::Qt_5_9);

    qint32 count = 0;
    in >> count;

    for (qint32 i = 0; i < count; ++i) {
        QString taskId;
        QDateTime deletedDateTime;

        in >> taskId;
        in >> deletedDateTime;
        DeletedTaskData newTask;
        newTask.taskId = taskId;
        newTask.timeDeleted = deletedDateTime;
        taskList->append(newTask);
    }

    file.close();
    return true;
}
bool overwritingFile(const QString &fileName, const QList<TaskForm*>* taskList)
{

    // Формируем полный путь к файлу и сохраняем его в члене класса
    QString m_filePath = fullFilePath(fileName);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Не удалось открыть файл для записи:" << m_filePath;
        return false;
    }

    QDataStream out(&file);

    out.setVersion(QDataStream::Qt_5_9);

    // Записываем количество задач
    out << static_cast<qint32>(taskList->size());

    for (const TaskForm* task : *taskList) {
        if (!task) continue;
        out << task->getTaskId();
        out << task->isDone();
        out << task->getTask();
        out << task->getDeadline();
        out << task->getDeadlineDateTime();
        out << task->getChangedDateTime();
        out << task->syncStatus();
    }

    file.close();
    return true;
}

bool loadFromFile(const QString &fileName, QList<TaskForm*>* taskList, QWidget *parent)
{
    QString m_filePath = fullFilePath(fileName);

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл для чтения:" << m_filePath;
        return false;
    }

    QDataStream in(&file);

    in.setVersion(QDataStream::Qt_5_9);

    qint32 count = 0;
    in >> count;

    for (qint32 i = 0; i < count; ++i) {
        QString taskId;
        bool done;
        QString task;
        QString deadline;
        QDateTime deadlineDeadTime;
        QDateTime changedDateTime;
        TaskForm::SyncStatus syncStatus;

        in >> taskId;
        in >> done;
        in >> task;
        in >> deadline;
        in >> deadlineDeadTime;
        in >> changedDateTime;
        in >> syncStatus;

        TaskForm* newTask = new TaskForm(parent);
        newTask->setTaskId(taskId);
        newTask->setDone(done);
        newTask->setTask(task);
        newTask->setDeadline(deadline);
        newTask->setDeadlineDateTime(deadlineDeadTime);
        newTask->setChangedDateTime(changedDateTime);
        newTask->setSyncStatus(syncStatus);
        qDebug() << "DeadlineDateTime:" << deadlineDeadTime;
        qDebug() << "ChangedDateTime:" << changedDateTime;
        taskList->append(newTask);
    }

    file.close();
    return true;
}
