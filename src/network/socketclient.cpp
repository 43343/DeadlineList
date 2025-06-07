#include "socketclient.h"
#include "../binarydatahandler.h"
#include <QJsonArray>
#include <QDateTime>

SocketClient::SocketClient(const QString &host, SecureStorage* secureStorage, const quint16 port, QList<TaskForm*>* taskList, QObject *parent) :
    QObject(parent),
    m_socket(new QTcpSocket(this)),
    m_host(host),
    m_port(port),
    m_taskList(taskList),
    deletedTaskList(new QList<DeletedTaskData>),
    m_secureStorage(secureStorage)
{
    loadFromFile("deletedTasks",deletedTaskList);
    connect(m_socket, &QTcpSocket::connected, this, &SocketClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &SocketClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &SocketClient::onReadyRead);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SocketClient::checkConnection);
    m_timer->start(5000);
    reconnect();
}
void SocketClient::syncTasksWithServer()
{
    if(!m_statusAuthorization || m_token.isEmpty() || m_requestTaskChangeSent) return;

    QJsonObject request;
    request["type"] = "sync_all_tasks";
    request["session"] = m_token;
    request["user_id"] = m_userId;

    m_requestQueue.enqueue({
        request,
        [this](const QJsonObject& response) {
            if(response.contains("tasks")) {
                                    if(m_requestTaskChangeSent) return;
                                    const QJsonArray& serverTasks = response["tasks"].toArray();
                                    QMap<QString, TaskForm*> localTasks;
                                    for(auto* task : *m_taskList) {
                                        localTasks[task->getTaskId()] = task;
                                    }

                                    for(const QJsonValue& taskValue : serverTasks) {
                                        QJsonObject serverTask = taskValue.toObject();
                                        QString taskId = serverTask["task_id"].toString();

                                        if(localTasks.contains(taskId)) {
                                            // Разрешение конфликтов
                                            TaskForm* localTask = localTasks[taskId];
                                            QDateTime serverChanged = QDateTime::fromString(
                                                serverTask["changed"].toString(), Qt::ISODate);
                                            QDateTime localChanged = localTask->getChangedDateTime();

                                            if(serverChanged > localChanged) {
                                                localTask->setTask(serverTask["task"].toString());
                                                localTask->setDone(serverTask["done"].toBool());
                                                localTask->setDeadline(serverTask["date"].toString());
                                                localTask->setDeadlineDateTime(QDateTime::fromString(serverTask["datetime"].toString(), Qt::ISODate));
                                                localTask->setChangedDateTime(serverChanged);
                                                localTask->setSyncStatus(TaskForm::Synced);
                                            }
                                        } else {
                                            TaskForm* newTask = new TaskForm();
                                            newTask->setTaskId(serverTask["task_id"].toString());
                                            newTask->setDone(serverTask["done"].toBool());
                                            newTask->setTask(serverTask["task"].toString());
                                            newTask->setDeadline(serverTask["date"].toString());
                                            newTask->setDeadlineDateTime(QDateTime::fromString(serverTask["datetime"].toString(), Qt::ISODate));
                                            newTask->setChangedDateTime(QDateTime::fromString(serverTask["changed"].toString(), Qt::ISODate));
                                            newTask->setSyncStatus(TaskForm::Synced);
                                            m_taskList->append(newTask);
                                            emit newTaskSynced(newTask);
                                        }
                                    }
                                    QList<QString> serverTaskIds;
                                    for(const QJsonValue& taskValue : serverTasks) {
                                        serverTaskIds.append(taskValue.toObject()["task_id"].toString());
                                    }

                                    for(auto it = localTasks.begin(); it != localTasks.end(); ++it) {
                                        if(!serverTaskIds.contains(it.key())) {
                                            it.value()->deleteLater();
                                            m_taskList->removeAll(it.value());
                                        }
                                    }
                                    overwritingFile("tasks", m_taskList);
            }
        }
    });

    if(!m_isRequestPending) sendNextRequest();
}

void SocketClient::syncTasks(QList<TaskForm*> tasks) {
    if(!m_statusAuthorization || m_token.isEmpty()) return;
        for(auto task : tasks) {
            switch(task->syncStatus()) {
            case TaskForm::Modified:
                sendTaskUpdate(task);
                break;
            case TaskForm::Deleted:
                sendTaskDeletion(task->getTaskId(), task->getChangedDateTime());
                break;
            default: break;
            }
        }
}

void SocketClient::sendTaskUpdate(TaskForm* task) {
    if(m_socket->state() == QAbstractSocket::ConnectedState)
    {
    qDebug() << "Отправляю запрос обновления задачи";
    QJsonObject obj;
    obj["type"] = "task_update";
    obj["session"] = m_token;
    obj["task_id"] = task->getTaskId();
    obj["task"] = task->getTask();
    obj["done"] = task->isDone();
    obj["date"] = task->getDeadline();
    obj["datetime"] = task->getDeadlineDateTime().toString(Qt::ISODate);
    obj["changed"] = task->getChangedDateTime().toString(Qt::ISODate);
    obj["user_id"] = m_userId;
    m_requestTaskChangeSent = true;

    m_requestQueue.enqueue({
        obj,
        [this, task](const QJsonObject& response) {
            if(response["status"] == "ok") {
                task->setSyncStatus(TaskForm::Synced);
                overwritingFile("tasks", m_taskList);
            } else {
                qWarning() << "Sync failed for task:" << task->getTaskId();
            }
            qDebug() << "Запрос обновления задачи выполнен";
            m_requestTaskChangeSent = false;
        }
    });

    if(!m_isRequestPending) sendNextRequest();
    }
}

void SocketClient::sendTaskDeletion(const QString& taskId, const QDateTime& dateTime) {
    if(m_socket->state() == QAbstractSocket::ConnectedState)
    {
    QJsonObject obj;
    obj["type"] = "task_delete";
    obj["session"] = m_token;
    obj["task_id"] = taskId;
    obj["user_id"] = m_userId;
    obj["changed"] = dateTime.toString(Qt::ISODate);
    m_requestTaskChangeSent = true;

    m_requestQueue.enqueue({
        obj,
        [this, taskId](const QJsonObject& response) {
            m_requestTaskChangeSent = false;
        }
    });

    if(!m_isRequestPending) sendNextRequest();
    }
    else {
        DeletedTaskData delTask;
        delTask.taskId = taskId;
        delTask.timeDeleted = dateTime;
        deletedTaskList->append(delTask);
        overwritingFile("deletedTasks",deletedTaskList);
    }
}
void SocketClient::sendCodeRegisterUser(const QString &email)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "registration_send_code";
        obj["email"] = email;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                if(response["status"] == "ok") {
                    emit sendCodeRegistrationSuccessfully();
                } else {
                    emit sendCodeRegistrationError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    } else {
        emit sendCodeRegistrationError("Ошибка подключения к серверу: \"Socket operation timed out\"");
    }
}
void SocketClient::checkCodeRegisterUser(const QString &email, const QString& password, const QString& code)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "registration_check_code";
        obj["email"] = email;
        obj["password"] = password;
        obj["code"] = code;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
                    m_statusAuthorization = true;
                    m_secureStorage->store("token", m_token.toUtf8());
                    m_secureStorage->store("user", m_userId.toUtf8());
                    emit checkCodeRegistrationSuccessfully();
                } else {
                    m_token.clear();
                    m_userId.clear();
                    m_secureStorage->clear();
                    m_statusAuthorization = false;
                    emit checkCodeRegistrationError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    } else {
        emit sendCodeRegistrationError("Ошибка подключения к серверу: \"Socket operation timed out\"");
    }
}
void SocketClient::sendCodeEmailResetPassword(const QString &email)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "reset_password_send_code";
        obj["email"] = email;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
                    emit sendCodeEmailResetPasswordSuccessfully();
                } else {
                    emit sendCodeEmailResetPasswordError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    } else {
        emit sendCodeEmailResetPasswordError("Ошибка подключения к серверу: \"Socket operation timed out\"");
    }
}
void SocketClient::checkCodeEmailResetPassword(const QString &code)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "reset_password_check_code";
        obj["code"] = code;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                if(response["status"] == "ok") {
                    emit checkCodeEmailResetPasswordSuccessfully();
                } else {
                    emit checkCodeEmailResetPasswordError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    } else {
        emit checkCodeEmailResetPasswordError("Ошибка подключения к серверу: \"Socket operation timed out\"");
    }
}
void SocketClient::confirmResetNewPassword(const QString& password)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "reset_password_new";
        obj["new_password"] = password;
        obj["user_id"] = m_userId;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                if(response["status"] == "ok") {
                    m_statusAuthorization = true;
                    m_secureStorage->store("token", m_token.toUtf8());
                    m_secureStorage->store("user", m_userId.toUtf8());
                    syncTasks(*m_taskList);
                    emit confirmResetNewPasswordSuccessfully();
                } else {
                    m_token.clear();
                    m_userId.clear();
                    m_secureStorage->clear();
                    m_statusAuthorization = false;
                    emit confirmResetNewPasswordError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    } else {
        emit confirmResetNewPasswordError("Ошибка подключения к серверу: \"Socket operation timed out\"");
    }
}
void SocketClient::authorizationUser(const QString &email, const QString &password)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "authorization";
        obj["email"] = email;
        obj["password"] = password;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
                    m_statusAuthorization = true;
                    m_secureStorage->store("token", m_token.toUtf8());
                    m_secureStorage->store("user", m_userId.toUtf8());
                    syncTasks(*m_taskList);
                    emit authorizationSuccessfully();
                } else {
                    m_token.clear();
                    m_userId.clear();
                    m_secureStorage->clear();
                    m_statusAuthorization = false;
                    emit authorizationError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    } else {
        emit authorizationError("Ошибка подключения к серверу: \"Socket operation timed out\"");
    }
}
void SocketClient::changePasswordUser(const QString &oldPassword, const QString &newPassword)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "change_password";
        obj["user_id"] = m_userId;
        obj["session"] = m_token;
        obj["old_password"] = oldPassword;
        obj["new_password"] = newPassword;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
                    emit changePasswordSuccessfully();
            } else {
                    emit changePasswordError(response["message"].toString());
            }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    } else {
        emit changePasswordError("Ошибка подключения к серверу: \"Socket operation timed out\"");
    }
}
void SocketClient::exitUser()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj["type"] = "delete_session";
        obj["session"] = m_token;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                if(response["status"] == "ok") {
                    m_token.clear();
                    m_userId.clear();
                    m_secureStorage->clear();
                    m_statusAuthorization = false;
                    emit exitSuccessfully();
                } else {
                    emit exitError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    }
}

void SocketClient::checkConnection()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        reconnect();
    } else {
        if(m_statusAuthorization && !m_token.isEmpty())
        {
            qDebug() << m_requestTaskChangeSent;
            QJsonObject request;
            request["type"] = "check_session";
            request["session"] = m_token;

            QJsonDocument doc(request);
            QByteArray data = doc.toJson();

            m_requestQueue.enqueue({
                request,
                [this](const QJsonObject& response) {
                    m_token = response["session"].toString();
                    m_userId = response["user_id"].toString();
                    if(response["status"] == "ok") {
                        m_statusAuthorization = true;
                        m_secureStorage->store("token", m_token.toUtf8());
                        m_secureStorage->store("user", m_userId.toUtf8());
                        syncTasksWithServer();
                        emit validSession();
                    } else {
                        m_token.clear();
                        m_userId.clear();
                        m_secureStorage->clear();
                        m_statusAuthorization = false;
                        emit invalidSession();
                    }
                }
            });

            if(!m_isRequestPending) sendNextRequest();
        }
    }
}
void SocketClient::sendNextRequest() {
    if(m_requestQueue.isEmpty() || m_isRequestPending) return;

    m_isRequestPending = true;
    Request& request = m_requestQueue.head();

    QJsonDocument doc(request.data);
    m_socket->write(doc.toJson());
}
void SocketClient::onConnected()
{
    if(!m_token.isEmpty())
    {
        QJsonObject request;
        request["type"] = "check_session";
        request["session"] = m_token;

        QJsonDocument doc(request);
        QByteArray data = doc.toJson();

        m_requestQueue.enqueue({
            request,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
                    m_statusAuthorization = true;
                    m_secureStorage->store("token", m_token.toUtf8());
                    m_secureStorage->store("user", m_userId.toUtf8());
                    loadFromFile("deletedTasks", deletedTaskList);
                    if(!deletedTaskList->isEmpty())
                    {
                        for(const auto& delTask : *deletedTaskList) {
                            sendTaskDeletion(delTask.taskId, delTask.timeDeleted);
                        }
                        deletedTaskList->clear();
                        deleteFile("deletedTasks");
                    }

                    // Синхронизация измененных задач
                    if(!m_taskList->isEmpty()) {
                        syncTasks(*m_taskList);
                    }
                    emit validSession();
                } else {
                    m_token.clear();
                    m_userId.clear();
                    m_secureStorage->clear();
                    m_statusAuthorization = false;
                    deletedTaskList->clear();
                    deleteFile("deletedTasks");
                    emit invalidSession();
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
    }
    emit connected();
}

void SocketClient::onDisconnected()
{
    m_requestTaskChangeSent = false;
    m_isRequestPending = false;
    emit disconnected();
}

void SocketClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
}
void SocketClient::onReadyRead()
{
    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_5_15);

    while (true) {
        if (m_expectedSize == 0) {
            if (m_socket->bytesAvailable() < sizeof(quint32)) break;
            in >> m_expectedSize;
        }

        if (m_socket->bytesAvailable() < m_expectedSize) break;

        QByteArray data = m_socket->read(m_expectedSize);
        m_expectedSize = 0;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if(doc.isObject())
        {
            if (error.error != QJsonParseError::NoError) {
                qDebug() << "JSON error:" << error.errorString();
                continue;
            }

            if(!m_requestQueue.isEmpty()) {
                auto nextRequest = m_requestQueue.dequeue();
                if(nextRequest.handler) nextRequest.handler(doc.object());
                m_isRequestPending = false;
                sendNextRequest();
            }
            else
            {
                m_isRequestPending = false;
            }
        }
    }
}
bool SocketClient::getStatusAuthorization()
{
    return m_statusAuthorization;
}
bool SocketClient::getConnected()
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}
void SocketClient::reconnect()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState)
        return;
    m_socket->abort();
    m_socket->connectToHost(m_host, m_port);
}
void SocketClient::setToken(const QString& token)
{
    m_token = token;
}
void SocketClient::setUserId(const QString& userId)
{
    m_userId = userId;
}
