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

class SocketClient : public QObject
{
    Q_OBJECT
public:
    SocketClient(const QString &host, const quint16 port, KeyChainClass* keychain, QObject *parent = nullptr);
    void registerUser(const QString &email, const QString &password);
    void authorizationUser(const QString &email, const QString &password);
    void changePasswordUser(const QString &oldPassword, const QString &newPassword);
    void exitUser();
    bool getStatusAuthorization();
    bool getConnected();
    void setToken(const QString& token);
    void setUserId(const QString& userId);
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
    void registrationSuccessfully();
    void registrationError(const QString& error);
    void authorizationSuccessfully();
    void authorizationError(const QString& error);
    void exitSuccessfully();
    void exitError(const QString& error);
    void changePasswordSuccessfully();
    void changePasswordError(const QString& error);
private:
    QTcpSocket *m_socket;
    QTimer *m_timer;
    QString m_host;
    quint16 m_port;
    QString m_token;
    QString m_userId;
    KeyChainClass* m_keychain;
    bool mStatusAuthorization = false;
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
private:
    void reconnect();
};

#endif // SOCKETCLIENT_H
