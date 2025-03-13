#include "socketclient.h"
#include "binarydatahandler.h"

SocketClient::SocketClient(const QString &host, const quint16 port, KeyChainClass* keychain, QObject *parent) : QObject(parent), m_host(host), m_port(port), m_keychain(keychain)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &SocketClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &SocketClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &SocketClient::onReadyRead);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SocketClient::checkConnection);
    m_timer->start(5000);
    reconnect();
}

void SocketClient::registerUser(const QString &email, const QString &password)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        // Формируем JSON-запрос
        QJsonObject obj;
        obj["type"] = "registration";
        obj["email"] = email;
        obj["password"] = password;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_socket->write(data);
        qDebug() << "Запрос регистрации отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
    }
}
void SocketClient::authorizationUser(const QString &email, const QString &password)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        // Формируем JSON-запрос
        QJsonObject obj;
        obj["type"] = "authorization";
        obj["email"] = email;
        obj["password"] = password;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_socket->write(data);
        qDebug() << "Запрос регистрации отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
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
        m_socket->write(data);
        qDebug() << "Запрос удаления сессии отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
    }
}
void SocketClient::exitUser()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        // Формируем JSON-запрос
        QJsonObject obj;
        obj["type"] = "delete_session";
        obj["session"] = m_token;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        m_socket->write(data);
        qDebug() << "Запрос удаления сессии отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
    }
}

void SocketClient::checkConnection()
{
    // Если сокет не в состоянии "Connected", пробуем переподключиться
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Соединение потеряно. Пытаемся переподключиться...";
        reconnect();
    } else {
        // При необходимости можно отправлять heartbeat-сообщения
        if(mStatusAuthorization && !m_token.isEmpty())
        {
            QJsonObject request;
            request["type"] = "check_session";
            qDebug() << m_token;
            request["session"] = m_token; // Передаём сохранённый токен

            QJsonDocument doc(request);
            QByteArray data = doc.toJson();

            // Отправляем запрос на сервер
            m_socket->write(data);
        }
        qDebug() << "Соединение активно.";
    }
}
void SocketClient::onConnected()
{
    qDebug() << "Успешно подключились к серверу.";
    if(!m_token.isEmpty())
    {
        QJsonObject request;
        request["type"] = "check_session";
        qDebug() << m_token;
        const QString token = m_token;
        request["session"] = token; // Передаём сохранённый токен

        QJsonDocument doc(request);
        QByteArray data = doc.toJson();

        // Отправляем запрос на сервер
        m_socket->write(data);
    }
    emit connected();
}

// Слот, вызываемый при разрыве соединения
void SocketClient::onDisconnected()
{
    qDebug() << "Соединение с сервером разорвано.";
    emit disconnected();
}

// Слот для обработки ошибок
void SocketClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    qDebug() << "Ошибка сокета:" << m_socket->errorString();
}
void SocketClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        QString status = obj["status"].toString();
        m_token = obj["session"].toString();
        m_userId = obj["user_id"].toString();
        QString message = obj["message"].toString();
        qDebug() << "Ответ сервера:" << status << "-" << message;
        qDebug() << "Ответ сервера:" << status << "-" << message << "-" << type << "-" << m_token << "-" << m_userId;
        if (type == "change_password_reply") {
            if(status == "ok") {
                emit changePasswordSuccessfully();
            } else {
                qDebug() << "Не удалось изменить пароль";
                emit changePasswordError(message);
            }
        }
        if (type == "delete_session_reply") {
            if(status == "ok") {
                m_token.clear();
                m_userId.clear();
                m_keychain->writeUserId(m_userId);
                m_keychain->writeToken(m_token);
                mStatusAuthorization = false;
                emit exitSuccessfully();
            } else {
                qDebug() << "Не удалось удалить сессию";
                emit exitError();
            }
        }
        if (type == "check_session_reply") {
            if(status == "ok") {
                qDebug() << "mUserId: " << m_userId;
                mStatusAuthorization = true;
                m_keychain->writeToken(m_token);
                emit validSession();
            } else {
                // Сессия не валидна, например, необходимо перелогиниться
                m_token.clear();
                m_userId.clear();
                m_keychain->writeUserId(m_userId);
                m_keychain->writeToken(m_token);
                mStatusAuthorization = false;
                emit invalidSession();
            }
        }
        if (type == "authorization_reply") {
            if(status == "ok") {
                mStatusAuthorization = true;
                m_keychain->writeUserId(m_userId);
                m_keychain->writeToken(m_token);
                emit authorizationSuccessfully();
            } else {
                m_token.clear();
                m_userId.clear();
                mStatusAuthorization = false;
                m_keychain->writeUserId(m_userId);
                m_keychain->writeToken(m_token);
                emit authorizationError(message);
            }
        }
        if (type == "registration_reply") {
            if(status == "ok") {
                mStatusAuthorization = true;
                m_keychain->writeUserId(m_userId);
                m_keychain->writeToken(m_token);
                emit registrationSuccessfully();
            } else {
                // Сессия не валидна, например, необходимо перелогиниться
                m_token.clear();  // сброс токена
                m_userId.clear();
                mStatusAuthorization = false;
                m_keychain->writeUserId(m_userId);
                m_keychain->writeToken(m_token);
                emit registrationError(message);
            }
        }
    } else {
        qDebug() << "Получен некорректный ответ:" << data;
        mStatusAuthorization = false;
    }
}
bool SocketClient::getStatusAuthorization()
{
    return mStatusAuthorization;
}
bool SocketClient::getConnected()
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}
void SocketClient::reconnect()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState)
        return;

    // Закрываем предыдущее соединение (если было)
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
