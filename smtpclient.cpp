#include "smtpclient.h"

SMTPClient::SMTPClient(const QString &host, quint16 port,   const QString &username, const QString &password, QObject *parent) :
    QObject(parent),
    m_host(host),
    m_port(port),
    m_username(username),
    m_password(password)
{

}

bool SMTPClient::sendCommand(const QString &cmd, int timeout)
{
    qDebug() << ">>" << cmd.trimmed();
    m_socket.write(cmd.toUtf8());
    if (!m_socket.waitForBytesWritten(timeout)) {
        qWarning() << "Ошибка при отправке команды:" << m_socket.errorString();
        return false;
    }

    if (!m_socket.waitForReadyRead(timeout)) {
        qWarning() << "Нет ответа на команду:" << cmd;
        return false;
    }

    while (m_socket.canReadLine()) {
        QByteArray responseLine = m_socket.readLine();
        qDebug() << "<<" << responseLine.trimmed();
        if (responseLine.length() >= 4 && responseLine.at(3) == ' ')
            break;
    }
    return true;
}
void SMTPClient::sendMail(const QString &from, const QString &to, const QString &subject, const QString &body)
{
    qDebug() << QSslSocket::supportsSsl();
    m_socket.connectToHostEncrypted(m_host, m_port);
    if (!m_socket.waitForEncrypted(5000)) {
        qWarning() << "Не удалось установить защищённое соединение:" << m_socket.errorString();
        return;
    }

    if (!m_socket.waitForReadyRead(3000)) {
        qWarning() << "Нет данных от сервера";
        return;
    }
    qDebug() << ">> Получено:" << m_socket.readAll();

    if (!sendCommand("EHLO localhost\r\n"))
        return;

    if (!sendCommand("AUTH LOGIN\r\n"))
        return;

    QString encodedUsername = m_username.toLocal8Bit().toBase64();
    if (!sendCommand(encodedUsername + "\r\n"))
        return;

    QString encodedPassword = m_password.toLocal8Bit().toBase64();
    if (!sendCommand(encodedPassword + "\r\n"))
        return;

    if (!sendCommand("MAIL FROM:<" + from + ">\r\n"))
        return;

    if (!sendCommand("RCPT TO:<" + to + ">\r\n"))
        return;

    if (!sendCommand("DATA\r\n"))
        return;

    QString data;
    data += "Subject: " + subject + "\r\n";
    data += "From: Deadlinelist <" + from + ">\r\n";
    data += "To: " + to + "\r\n";
    data += "MIME-Version: 1.0\r\n";
    data += "Content-Type: text/plain; charset=\"utf-8\"\r\n";
    data += "\r\n";
    data += body + "\r\n";
    data += ".\r\n";

    if (!sendCommand(data))
        return;

    sendCommand("QUIT\r\n", 1000);

    m_socket.disconnectFromHost();

    qDebug() << "Письмо успешно отправлено.";
}
