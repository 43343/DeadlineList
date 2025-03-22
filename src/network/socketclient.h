#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H
#include <QTcpSocket>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QQueue>
#include "keychainclass.h"
#include "../tasks/deletedtaskdata.h"
#include "../tasks/taskform.h"

class SocketClient : public QObject
{
    Q_OBJECT
public:
    SocketClient(const QString &host, const quint16 port, KeyChainClass* keychain, QList<TaskForm*>* taskList, QObject *parent = nullptr);
    void sendCodeRegisterUser(const QString &email);
    void checkCodeRegisterUser(const QString &email, const QString &password, const QString& code);
    void authorizationUser(const QString &email, const QString &password);
    void changePasswordUser(const QString &oldPassword, const QString &newPassword);
    void sendCodeEmailResetPassword(const QString& email);
    void checkCodeEmailResetPassword(const QString& code);
    void confirmResetNewPassword(const QString& password);
    void exitUser();
    bool getStatusAuthorization();
    bool getConnected();
    void setToken(const QString& token);
    void setUserId(const QString& userId);
    void syncTasks(QList<TaskForm*> tasks);
    void syncTasksWithServer();
private slots:
    void checkConnection();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();
signals:
    void connected();
    void disconnected();
    void validSession();
    void invalidSession();
    void sendCodeRegistrationSuccessfully();
    void sendCodeRegistrationError(const QString& error);
    void checkCodeRegistrationSuccessfully();
    void checkCodeRegistrationError(const QString& error);
    void authorizationSuccessfully();
    void authorizationError(const QString& error);
    void exitSuccessfully();
    void exitError(const QString& error);
    void changePasswordSuccessfully();
    void changePasswordError(const QString& error);
    void sendCodeEmailResetPasswordSuccessfully();
    void sendCodeEmailResetPasswordError(const QString& error);
    void checkCodeEmailResetPasswordSuccessfully();
    void checkCodeEmailResetPasswordError(const QString& error);
    void confirmResetNewPasswordSuccessfully();
    void confirmResetNewPasswordError(const QString& error);
    void deleteTaskSuccessfully();
    void deleteTaskError(const QString& error);
    void newTaskSynced(TaskForm* newTaskForm);
private:
    QTcpSocket *m_socket;
    QTimer *m_timer;
    QString m_host;
    quint16 m_port;
    QString m_token;
    QString m_userId;
    KeyChainClass* m_keychain;
    bool m_requestTaskChangeSent = false;
    bool m_statusAuthorization = false;
    QList<DeletedTaskData>* deletedTaskList;
    void sendTaskUpdate(TaskForm* task);
    void sendTaskDeletion(const QString& taskId, const QDateTime& dateTime);
    QList<TaskForm*>* m_taskList;
private:
    struct Request {
        QJsonObject data;
        std::function<void(const QJsonObject&)> handler;
    };

    QQueue<Request> m_requestQueue;
    bool m_isRequestPending = false;
    QByteArray m_buffer;
    quint32 m_expectedSize = 0;

    void sendNextRequest();
    void syncWithServer();
private:
    void reconnect();
};

#endif // SOCKETCLIENT_H
