#include "binarydatahandler.h"
#include <QFile>
#include <QDir>
#include <QDataStream>
#include <QDebug>
#include <QStandardPaths>

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
        out << task->isDone();
        out << task->getTask();
        out << task->getDeadline();
        out << task->getDeadlineDateTime();
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
        bool done;
        QString task;
        QString deadline;
        QDateTime deadlineDeadTime;

        in >> done;
        in >> task;
        in >> deadline;
        in >> deadlineDeadTime;

        TaskForm* newTask = new TaskForm(parent);
        newTask->setDone(done);
        newTask->setTask(task);
        newTask->setDeadline(deadline);
        newTask->setDeadlineDateTime(deadlineDeadTime);
        taskList->append(newTask);
    }

    file.close();
    return true;
}
