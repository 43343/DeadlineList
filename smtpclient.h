#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H
#include <QObject>
#include <QSslSocket>

class SMTPClient : public QObject
{
    Q_OBJECT
public:
    explicit SMTPClient(const QString &host, quint16 port, const QString &username, const QString &password, QObject *parent = nullptr);

    void sendMail(const QString &from, const QString &to, const QString &subject, const QString &body);

private:
    bool sendCommand(const QString &cmd, int timeout = 3000);
private:
    QString m_host;
    quint16 m_port;
    QString m_username;
    QString m_password;
    QSslSocket m_socket;
};

#endif // SMTPCLIENT_H
