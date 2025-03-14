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
    bool isEmailRegistered(const QString &email);
    bool resetPassword(const QString &userId, const QString &newPassword);
    bool findUserByEmail(const QString &email, QString &userId);
    bool registerNewUser(const QString &email, const QString &password, int &userId);
    bool authorizationUser(const QString &email, const QString &password, int &userId);
    bool changePassword(const QString &token, const QString &userId, const QString& oldPassword, const QString& newPassword);
private:
    QString createSession(int userId);
    bool checkSessionInDatabase(const QString& token);
    bool deleteSession(const QString& token);
    void sendResponse(QTcpSocket* socket, const QJsonObject& response);
    CryptData cryptData;
};

#endif // SERVER_H
