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
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
                    mStatusAuthorization = true;
                    m_keychain->writeUserId(m_userId);
                    m_keychain->writeToken(m_token);
                    emit registrationSuccessfully();
                } else {
                    m_token.clear();
                    m_userId.clear();
                    mStatusAuthorization = false;
                    m_keychain->writeUserId(m_userId);
                    m_keychain->writeToken(m_token);
                    emit registrationError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
        qDebug() << "Запрос регистрации отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
        emit registrationError("Ошибка подключения к серверу: \"Socket operation timed out\"");
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
        m_requestQueue.enqueue({
            obj,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
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
                    emit authorizationError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
        qDebug() << "Запрос регистрации отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
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
                    qDebug() << "Не удалось изменить пароль";
                    emit changePasswordError(response["message"].toString());
            }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
        qDebug() << "Запрос удаления сессии отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
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
                    m_keychain->writeUserId(m_userId);
                    m_keychain->writeToken(m_token);
                    mStatusAuthorization = false;
                    emit exitSuccessfully();
                } else {
                    qDebug() << "Не удалось удалить сессию";
                    emit exitError(response["message"].toString());
                }
            }
        });

        if(!m_isRequestPending) sendNextRequest();
        qDebug() << "Запрос удаления сессии отправлен:" << data;
    } else {
        qDebug() << "Нет соединения с сервером. Регистрация не может быть выполнена.";
    }
}

void SocketClient::checkConnection()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Соединение потеряно. Пытаемся переподключиться...";
        reconnect();
    } else {
        if(mStatusAuthorization && !m_token.isEmpty())
        {
            QJsonObject request;
            request["type"] = "check_session";
            qDebug() << m_token;
            request["session"] = m_token;

            QJsonDocument doc(request);
            QByteArray data = doc.toJson();

            m_requestQueue.enqueue({
                request,
                [this](const QJsonObject& response) {
                    m_token = response["session"].toString();
                    m_userId = response["user_id"].toString();
                    if(response["status"] == "ok") {
                        qDebug() << "mUserId: " << m_userId;
                        mStatusAuthorization = true;
                        m_keychain->writeToken(m_token);
                        emit validSession();
                    } else {
                        m_token.clear();
                        m_userId.clear();
                        m_keychain->writeUserId(m_userId);
                        m_keychain->writeToken(m_token);
                        mStatusAuthorization = false;
                        emit invalidSession();
                    }
                }
            });

            if(!m_isRequestPending) sendNextRequest();
        }
        qDebug() << "Соединение активно.";
    }
}
void SocketClient::sendNextRequest() {
    if(m_requestQueue.isEmpty() || m_isRequestPending) return;

    m_isRequestPending = true;
    Request& request = m_requestQueue.head();

    QJsonDocument doc(request.data);
    m_socket->write(doc.toJson());
    qDebug() << "Запрос отправлен:" << doc.toJson();
}
void SocketClient::onConnected()
{
    qDebug() << "Успешно подключились к серверу.";
    if(!m_token.isEmpty())
    {
        QJsonObject request;
        request["type"] = "check_session";
        qDebug() << m_token;
        request["session"] = m_token;

        QJsonDocument doc(request);
        QByteArray data = doc.toJson();

        m_requestQueue.enqueue({
            request,
            [this](const QJsonObject& response) {
                m_token = response["session"].toString();
                m_userId = response["user_id"].toString();
                if(response["status"] == "ok") {
                    qDebug() << "mUserId: " << m_userId;
                    mStatusAuthorization = true;
                    m_keychain->writeToken(m_token);
                    emit validSession();
                } else {
                    m_token.clear();
                    m_userId.clear();
                    m_keychain->writeUserId(m_userId);
                    m_keychain->writeToken(m_token);
                    mStatusAuthorization = false;
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
    qDebug() << "Соединение с сервером разорвано.";
    emit disconnected();
}

void SocketClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    qDebug() << "Ошибка сокета:" << m_socket->errorString();
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

            m_isRequestPending = false;
            qDebug() << "doc " << doc.object();

            if(!m_requestQueue.isEmpty()) {
                auto nextRequest = m_requestQueue.dequeue();
                if(nextRequest.handler) nextRequest.handler(doc.object());
                else qDebug() << "not handler";
                sendNextRequest();
            }
            else
            {
                qDebug() << "очередь реквеста пустая";
            }
        }
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
