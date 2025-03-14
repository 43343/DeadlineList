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

    // Читаем ответ. Некоторые ответы состоят из нескольких строк.
    while (m_socket.canReadLine()) {
        QByteArray responseLine = m_socket.readLine();
        qDebug() << "<<" << responseLine.trimmed();
        // Если код ответа имеет пробел после кода, значит это последняя строка
        if (responseLine.length() >= 4 && responseLine.at(3) == ' ')
            break;
    }
    return true;
}
void SMTPClient::sendMail(const QString &from, const QString &to, const QString &subject, const QString &body)
{
    qDebug() << QSslSocket::supportsSsl();
    // Устанавливаем защищённое соединение с сервером
    m_socket.connectToHostEncrypted(m_host, m_port);
    if (!m_socket.waitForEncrypted(5000)) {
        qWarning() << "Не удалось установить защищённое соединение:" << m_socket.errorString();
        return;
    }

    // Читаем приветственное сообщение сервера
    if (!m_socket.waitForReadyRead(3000)) {
        qWarning() << "Нет данных от сервера";
        return;
    }
    qDebug() << ">> Получено:" << m_socket.readAll();

    // 1. EHLO
    if (!sendCommand("EHLO localhost\r\n"))
        return;

    // 2. AUTH LOGIN
    if (!sendCommand("AUTH LOGIN\r\n"))
        return;

    // 3. Передаём логин в виде Base64
    QString encodedUsername = m_username.toLocal8Bit().toBase64();
    if (!sendCommand(encodedUsername + "\r\n"))
        return;

    // 4. Передаём пароль в виде Base64
    QString encodedPassword = m_password.toLocal8Bit().toBase64();
    if (!sendCommand(encodedPassword + "\r\n"))
        return;

    // 5. MAIL FROM
    if (!sendCommand("MAIL FROM:<" + from + ">\r\n"))
        return;

    // 6. RCPT TO
    if (!sendCommand("RCPT TO:<" + to + ">\r\n"))
        return;

    // 7. DATA – начало передачи письма
    if (!sendCommand("DATA\r\n"))
        return;

    // 8. Формируем заголовки и тело письма.
    QString data;
    data += "Subject: " + subject + "\r\n";
    data += "From: Deadlinelist <" + from + ">\r\n";
    data += "To: " + to + "\r\n";
    data += "MIME-Version: 1.0\r\n";
    data += "Content-Type: text/plain; charset=\"utf-8\"\r\n";
    data += "\r\n";
    data += body + "\r\n";
    // Заканчиваем данные точкой в отдельной строке, согласно протоколу SMTP.
    data += ".\r\n";

    if (!sendCommand(data))
        return;

    // 9. QUIT – завершаем сеанс
    sendCommand("QUIT\r\n", 1000);

    m_socket.disconnectFromHost();

    qDebug() << "Письмо успешно отправлено.";
}
