#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "cryptdata.h"

class Server : public QTcpServer
{
public:
    Server();
private:
    QSqlDatabase db;
private:
    void incomingConnection(qintptr socketDescriptor) override;
    void processRequest(QTcpSocket *socket);

    void syncAllTasks(QJsonObject &obj, QTcpSocket *socket);
    void taskUpdate(QJsonObject &obj, QTcpSocket *socket);
    void taskDelete(QJsonObject &obj, QTcpSocket *socket);
    void resetPasswordSendCode(QJsonObject &obj, QTcpSocket *socket);
    void resetPasswordCheckCode(QJsonObject &obj, QTcpSocket *socket);
    void resetPasswordNew(QJsonObject &obj, QTcpSocket *socket);
    void changePassword(QJsonObject &obj, QTcpSocket *socket);
    void deleteSession(QJsonObject &obj, QTcpSocket *socket);
    void registrationSendCode(QJsonObject &obj, QTcpSocket *socket);
    void registrationCheckCode(QJsonObject &obj, QTcpSocket *socket);
    void authorization(QJsonObject &obj, QTcpSocket *socket);
    void checkSession(QJsonObject &obj, QTcpSocket *socket);

    bool isEmailRegistered(const QString &email);
    bool resetPassword(const QString &userId, const QString &newPassword);
    bool findUserByEmail(const QString &email, QString &userId);
    bool registerNewUser(const QString &email, const QString &password, int &userId);
    bool authorizationUser(const QString &email, const QString &password, int &userId);
    bool changePassword(const QString &token, const QString &userId, const QString& oldPassword, const QString& newPassword);
    QString generateCode();
private:
    QString createSession(int userId);
    bool checkSessionInDatabase(const QString& token);
    bool deleteSession(const QString& token);
    void sendResponse(QTcpSocket* socket, const QJsonObject& response);
    CryptData cryptData;
};

#endif // SERVER_H
