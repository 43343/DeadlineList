#include "server.h"
#include <QRandomGenerator>
#include <QJsonArray>
#include "smtpclient.h"
#include "smtpcredentials.h"

Server::Server()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("users.db");
    if (!db.open()) {
        qDebug() << "Ошибка открытия базы данных:" << db.lastError().text();
    } else {
        QSqlQuery query;
        // Создаем таблицу, если она не существует
        if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "email TEXT UNIQUE, "
                        "password TEXT)")) {
            qDebug() << "Ошибка создания таблицы:" << query.lastError().text();
        }
        if (!query.exec("CREATE TABLE IF NOT EXISTS sessions ("
                        "session_token TEXT PRIMARY KEY, "
                        "user_id INTEGER, "
                        "expires_at TEXT, "
                        "authorization_expires_at TEXT, "
                        "FOREIGN KEY(user_id) REFERENCES users(id))")) {
            qDebug() << "Ошибка создания таблицы sessions:" << query.lastError().text();
        }
        if (!query.exec("CREATE TABLE IF NOT EXISTS tasks ("
                        "id TEXT PRIMARY KEY, "
                        "user_id INTEGER, "
                        "done INTEGER, "
                        "task TEXT,"
                        "date TEXT,"
                        "datetime TEXT,"
                        "changed TEXT,"
                        "FOREIGN KEY(user_id) REFERENCES users(user_id))")) {
            qDebug() << "Ошибка создания таблицы tasks:" << query.lastError().text();
        }
    }
    if (!listen(QHostAddress::Any, 1234)) {
        qDebug() << "Сервер не может стартовать:" << errorString();
    }
    qDebug() << "Сервер запущен на порту:" << serverPort();
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket;
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        socket->deleteLater();
        return;
    }
    qDebug() << "Новое подключение:" << socketDescriptor;

    // Подключаем обработку данных от конкретного подключения
    connect(socket, &QTcpSocket::readyRead, [this, socket]() {
        processRequest(socket);
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}
void Server::processRequest(QTcpSocket *socket)
{
    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qDebug() << "Неверный формат данных:" << data;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    if (type == "sync_all_tasks")
    {
        syncAllTasks(obj, socket);
    }
    if (type == "task_update") {
        taskUpdate(obj, socket);
    }
    if (type == "task_delete") {
        taskDelete(obj, socket);
    }
    if(type == "reset_password_send_code")
    {
        resetPasswordSendCode(obj, socket);
    }
    if(type == "reset_password_check_code")
    {
        resetPasswordCheckCode(obj, socket);
    }
    if(type == "reset_password_new")
    {
        resetPasswordNew(obj, socket);
    }
    if (type == "change_password")
    {
        changePassword(obj, socket);
    }
    if (type == "delete_session")
    {
        deleteSession(obj, socket);
    }
    if (type == "registration_send_code") {
        registrationSendCode(obj, socket);
    }
    if (type == "registration_check_code") {
        registrationCheckCode(obj, socket);
    }
    if (type == "authorization") {
        authorization(obj, socket);
    }
    if (type == "check_session") {
        checkSession(obj, socket);
    }
}
void Server::syncAllTasks(QJsonObject &obj, QTcpSocket *socket)
{
    qDebug() << "Получен запрос синхронизации задач";
    QString userId = cryptData.encryptQString(obj["user_id"].toString());
    QJsonObject response;
    QJsonArray tasks;

    QSqlQuery query;
    query.prepare("SELECT * FROM tasks WHERE user_id = :user_id");
    query.bindValue(":user_id", userId.toInt());

    if(query.exec()) {
        while(query.next()) {
            QJsonObject task;
            task["task_id"] = query.value("id").toString();
            task["done"] = query.value("done").toBool();
            task["task"] = cryptData.decryptQString(query.value("task").toString());
            task["date"] = cryptData.decryptQString(query.value("date").toString());
            task["datetime"] = cryptData.decryptQString(query.value("datetime").toString());
            task["changed"] = cryptData.decryptQString(query.value("changed").toString());
            tasks.append(task);
        }
    }

    response["tasks"] = tasks;
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
}
void Server::taskUpdate(QJsonObject &obj, QTcpSocket *socket)
{
    QJsonObject response;
    QString userId = cryptData.encryptQString(obj["user_id"].toString());
    QString taskId = obj["task_id"].toString();

    QSqlDatabase::database().transaction();
    QSqlQuery checkQuery;
    checkQuery.prepare(
        "SELECT changed FROM tasks "
        "WHERE id = :id AND user_id = :user_id"
        );
    checkQuery.bindValue(":id", taskId);
    checkQuery.bindValue(":user_id", userId.toInt());

    if (checkQuery.exec() && checkQuery.next()) {
        QDateTime serverChanged = QDateTime::fromString(
            cryptData.decryptQString(checkQuery.value("changed").toString()),
            Qt::ISODate
            );
        QDateTime clientChanged = QDateTime::fromString(
            obj["changed"].toString(), Qt::ISODate
            );

        if (serverChanged > clientChanged) {
            qDebug() << "The task version on the server is newer.";
            return;
        }
    }
    try {
        qDebug() << "Получен запрос обновления задач";

        // Валидация
        if (!obj.contains("task") || !obj.contains("changed")) {
            throw std::runtime_error("Invalid task format");
        }

        QSqlQuery query;
        query.prepare(
            "INSERT OR REPLACE INTO tasks "
            "(id, user_id, done, task, date, datetime, changed) "
            "VALUES (:id, :user_id, :done, :task, :date, :datetime, :changed)"
            );

        query.bindValue(":id", taskId);
        query.bindValue(":user_id", userId.toInt());
        query.bindValue(":done", obj["done"].toBool());
        query.bindValue(":task", cryptData.encryptQString(obj["task"].toString())); // Исправлено task -> title
        query.bindValue(":date", cryptData.encryptQString(obj["date"].toString()));
        query.bindValue(":datetime", cryptData.encryptQString(
                                         QDateTime::fromString(obj["datetime"].toString(), Qt::ISODate)
                                             .toString(Qt::ISODate)
                                         ));
        query.bindValue(":changed", cryptData.encryptQString(
                                        QDateTime::fromString(obj["changed"].toString(), Qt::ISODate)
                                            .toString(Qt::ISODate)
                                        ));

        if (!query.exec()) {
            throw std::runtime_error(
                QString("Failed to update task: %1")
                    .arg(query.lastError().text())
                    .toStdString()
                );
        }
        query.prepare("SELECT * FROM tasks WHERE user_id = :user_id");
        query.bindValue(":user_id", userId.toInt());
        if(query.exec()) {
            QJsonArray tasks;
            while(query.next()) {
                QJsonObject task;
                task["task_id"] = query.value("id").toString();
                task["done"] = query.value("done").toBool();
                task["task"] = cryptData.decryptQString(query.value("task").toString());
                task["date"] = cryptData.decryptQString(query.value("date").toString());
                task["datetime"] = cryptData.decryptQString(query.value("datetime").toString());
                task["changed"] = cryptData.decryptQString(query.value("changed").toString());
                tasks.append(task);
            }
            response["tasks"] = tasks;
        }
    }
    catch (const std::exception& e)
    {
        QSqlDatabase::database().rollback();
        response["status"] = "error";
        response["message"] = QString("Sync failed: %1").arg(e.what());
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
}
void Server::taskDelete(QJsonObject &obj, QTcpSocket *socket)
{
    QJsonObject response;
    QString userId = cryptData.encryptQString(obj["user_id"].toString());
    QString taskId = obj["task_id"].toString();

    QSqlDatabase::database().transaction();
    QSqlQuery checkQuery;
    checkQuery.prepare(
        "SELECT changed FROM tasks "
        "WHERE id = :id AND user_id = :user_id"
        );
    checkQuery.bindValue(":id", taskId);
    checkQuery.bindValue(":user_id", userId.toInt());

    if (checkQuery.exec() && checkQuery.next()) {
        QDateTime serverChanged = QDateTime::fromString(
            cryptData.decryptQString(checkQuery.value("changed").toString()),
            Qt::ISODate
            );
        QDateTime clientChanged = QDateTime::fromString(
            obj["changed"].toString(), Qt::ISODate
            );

        if (serverChanged > clientChanged) {
            qDebug() << "The task version on the server is newer.";
            return;
        }
    }
    try {
        qDebug() << "Получен запрос удаления задач";
        QSqlQuery query;
        query.prepare(
            "DELETE FROM tasks "
            "WHERE id = :id AND user_id = :user_id"
            );
        query.bindValue(":id", taskId);
        query.bindValue(":user_id", userId.toInt());

        if (!query.exec()) {
            throw std::runtime_error(
                QString("Failed to delete task: %1")
                    .arg(query.lastError().text())
                    .toStdString()
                );
        }
    }
    catch (const std::exception& e)
    {
        QSqlDatabase::database().rollback();
        response["status"] = "error";
        response["message"] = QString("Sync failed: %1").arg(e.what());
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
}
void Server::resetPasswordSendCode(QJsonObject &obj, QTcpSocket *socket)
{
    QString email = obj["email"].toString();
    QJsonObject response;
    QString userId;
    if(findUserByEmail(email, userId))
    {
        QString resetCode = generateCode();
        socket->setProperty("code", resetCode);
        response["type"] = "reset_password_send_code_reply";
        response["status"] = "ok";
        response["user_id"] = userId;
        response["message"] = "The user was found by mail.";
        QString smtpHost = "smtp.yandex.ru";
        quint16 smtpPort = 465;
        QString emailOwn;
        QString password;
        loadSmtpCredentials(emailOwn, password);
        SMTPClient client(smtpHost, smtpPort, emailOwn, password);
        client.sendMail(emailOwn, email, "Код для восстановления пароля.", resetCode);
    }
    else
    {
        response["type"] = "reset_password_send_code_reply";
        response["status"] = "error";
        response["message"] = "The user was not found by mail.";
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
}
void Server::resetPasswordCheckCode(QJsonObject &obj, QTcpSocket *socket)
{
    QString providedCode = obj["code"].toString();

    QString storedCode = socket->property("code").toString();

    QJsonObject response;
    if (!storedCode.isEmpty() && (providedCode == storedCode)) {
        response["type"]    = "reset_password_check_code_reply";
        response["status"]  = "ok";
        response["message"] = "The reset code is correct.";
    } else {
        response["type"]    = "reset_password_check_code_reply";
        response["status"]  = "error";
        response["message"] = "Incorrect or expired reset code.";
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
    socket->flush();
}
void Server::resetPasswordNew(QJsonObject &obj, QTcpSocket *socket)
{
    QString userId = obj["user_id"].toString();
    QString newPassword = obj["new_password"].toString();

    QJsonObject response;
    if (resetPassword(userId, newPassword)) {
        QString sessionToken = createSession(userId.toInt());
        response["type"]    = "reset_password_new_reply";
        response["session"] = cryptData.decryptQString(sessionToken);
        response["user_id"] = userId;
        response["status"]  = "ok";
        response["message"] = "Password changed successfully.";
    } else {
        response["type"]    = "reset_password_new_reply";
        response["status"]  = "error";
        response["message"] = "Couldn't change password.";
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
}
void Server::deleteSession(QJsonObject &obj, QTcpSocket *socket)
{
    QString token = cryptData.encryptQString(obj["session"].toString());
    qDebug() << "Получен запрос удаления сесиии:" << token;
    QJsonObject response;
    if(deleteSession(token))
    {
        response["type"] = "delete_session_reply";
        response["status"] = "ok";
        response["message"] = "Delete session is successful.";
    }
    else
    {
        response["status"] = "error";
        response["type"] = "delete_session_reply";
        response["message"] = "Delete session error.";
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
}
void Server::changePassword(QJsonObject &obj, QTcpSocket *socket)
{
    QString userId = obj["user_id"].toString();
    QString token = obj["session"].toString();
    QString oldPassword = cryptData.encryptQString(obj["old_password"].toString());
    QString newPassword = obj["new_password"].toString();
    QJsonObject response;
    if(changePassword(token, userId, oldPassword, newPassword))
    {
        response["type"] = "change_password_reply";
        response["session"] = token;
        response["user_id"] = userId;
        response["status"] = "ok";
        response["message"] = "Password changed successfully.";
    }
    else
    {
        response["type"] = "change_password_reply";
        response["status"] = "error";
        response["session"] = token;
        response["user_id"] = userId;
        response["message"] = "The old password was entered incorrectly.";
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
}
void Server::registrationSendCode(QJsonObject &obj, QTcpSocket *socket)
{
    QString email = obj["email"].toString();
    qDebug() << "Получен запрос регистрации:" << email;

    QJsonObject response;
    if (isEmailRegistered(email)) {
        response["status"] = "failed";
        response["type"] = "registration_send_code_reply";
        response["message"] = "The user with this email already exists.";
    } else {
        QString code = generateCode();
        socket->setProperty("code", code);
        response["type"] = "registration_send_code_reply";
        response["status"] = "ok";
        response["message"] = "The email confirmation code has been sent successfully.";
        QString smtpHost = "smtp.yandex.ru";
        quint16 smtpPort = 465;
        QString emailOwn;
        QString password;
        loadSmtpCredentials(emailOwn, password);
        SMTPClient client(smtpHost, smtpPort, emailOwn, password);
        client.sendMail(emailOwn, email, "Код для восстановления пароля.", code);
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
}
void Server::registrationCheckCode(QJsonObject &obj, QTcpSocket *socket)
{
    QString email = obj["email"].toString();
    QString password = obj["password"].toString();
    QString providedCode = obj["code"].toString();
    QString storedCode = socket->property("code").toString();
    qDebug() << "Получен запрос регистрации:" << email;

    QJsonObject response;
    if (!storedCode.isEmpty() && (providedCode == storedCode)) {
        if (isEmailRegistered(email)) {
            response["status"] = "failed";
            response["type"] = "registration_check_code_reply";
            response["message"] = "The user with this email already exists.";
        } else {
            int userId = -1;
            if (registerNewUser(email, password, userId)) {
                QString sessionToken = createSession(userId);
                response["type"] = "registration_check_code_reply";
                response["session"] = cryptData.decryptQString(sessionToken);
                response["user_id"] = QString::number(userId);
                response["status"] = "ok";
                response["message"] = "Registration is successful.";
            } else {
                response["status"] = "failed";
                response["type"] = "registration_check_code_reply";
                response["message"] = "Registration error.";
            }
        }
    }
    else {
        response["type"]    = "registration_check_code_reply";
        response["status"]  = "error";
        response["message"] = "Incorrect or expired confirmation code.";
    }
    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
}
void Server::authorization(QJsonObject &obj, QTcpSocket *socket)
{
    QString email = obj["email"].toString();
    QString password = obj["password"].toString();
    qDebug() << "Получен запрос авторизации:" << email;

    QJsonObject response;
    int userId = -1;
    if (authorizationUser(email, password, userId)) {
        QString sessionToken = createSession(userId);
        response["type"] = "authorization_reply";
        response["session"] = cryptData.decryptQString(sessionToken);
        response["user_id"] = QString::number(userId);
        response["status"] = "ok";
        response["message"] = "Authorization is successful.";
    } else {
        response["status"] = "failed";
        response["type"] = "authorization_reply";
        response["message"] = "User with this email and password not found.";
    }

    QJsonDocument replyDoc(response);
    sendResponse(socket, response);
    qDebug() << replyDoc;
}
void Server::checkSession(QJsonObject &obj, QTcpSocket *socket)
{
    // Получаем переданный токен
    QString token = cryptData.encryptQString(obj["session"].toString());

    // Здесь добавьте логику проверки валидности токена.
    // Например, выполнить запрос к базе данных и проверить срок действия сессии.
    bool isValid = checkSessionInDatabase(token); // пример функции проверки.

    QJsonObject response;
    response["type"] = "check_session_reply";
    if (isValid) {
        QSqlQuery updateQuery;
        QDateTime newAuthorizationExpiresAt = QDateTime::currentDateTime().addMonths(1);
        updateQuery.prepare("UPDATE sessions SET authorization_expires_at = :authorization_expires_at WHERE session_token = :token");
        updateQuery.bindValue(":authorization_expires_at", cryptData.encryptQString(newAuthorizationExpiresAt.toString(Qt::ISODate)));
        updateQuery.bindValue(":token", token);
        if (updateQuery.exec()) {
        } else {
        }
        QSqlQuery selectQuery;
        selectQuery.prepare("SELECT user_id, expires_at FROM sessions WHERE session_token = :token");
        selectQuery.bindValue(":token", token);
        if (!selectQuery.exec()) {
        }
        int userId;
        if (selectQuery.next()) {
            QDateTime expiresAt = QDateTime::fromString(cryptData.decryptQString(selectQuery.value("expires_at").toString()), Qt::ISODate);
            userId = selectQuery.value("user_id").toInt();
            QDateTime now = QDateTime::currentDateTime();
            if(expiresAt < now)
            {
                deleteSession(token);
                token = createSession(userId);
            }
            if (updateQuery.exec()) {
            } else {
            }
        }
        response["status"] = "ok";
        response["session"] = cryptData.decryptQString(token);
        response["user_id"] = QString::number(userId);
        response["message"] = "The session is valid.";
    } else {
        response["status"] = "error";
        response["message"] = "The session is invalid.";
    }

    sendResponse(socket, response);
}
QString Server::generateCode()
{
    const QString symbols = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const int codeLength = 6;
    QString code;
    for (int i = 0; i < codeLength; ++i) {
        int index = QRandomGenerator::global()->bounded(symbols.length());
        code.append(symbols.at(index));
    }
    return code;
}
bool Server::checkSessionInDatabase(const QString& token)
{
    QSqlQuery query;
    query.prepare("SELECT authorization_expires_at FROM sessions WHERE session_token = :token");
    query.bindValue(":token", token);
    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса проверки сессии:" << query.lastError().text();
        return false;
    }

    // Если запись найдена, проверяем время истечения.
    if (query.next()) {
        QDateTime authoriazationExpiresAt = QDateTime::fromString(cryptData.decryptQString(query.value("authorization_expires_at").toString()), Qt::ISODate);
        QDateTime now = QDateTime::currentDateTime();
        if (authoriazationExpiresAt > now) {
            // Сессия валидна.
            return true;
        } else {
            deleteSession(token);
            return false;
        }
    } else {
        // Запись с таким токеном не найдена.
        qDebug() << "Запись с токеном не найдена: " << token;
        return false;
    }
}
bool Server::resetPassword(const QString &userId, const QString &newPassword)
{
    QSqlQuery query;

    // 1. Обновление пароля в базе данных.
    QString encryptedNewPassword = cryptData.encryptQString(newPassword);
    query.prepare("UPDATE users SET password = :newPassword WHERE id = :id");
    query.bindValue(":newPassword", encryptedNewPassword);
    query.bindValue(":id", userId);

    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса UPDATE:" << query.lastError().text();
        return false;
    }

    // 2. Удаление всех сессий пользователя.
    QSqlQuery sessionQuery;
    sessionQuery.prepare("DELETE FROM sessions WHERE user_id = :user_id");
    sessionQuery.bindValue(":user_id", userId);

    if (!sessionQuery.exec()) {
        qDebug() << "Ошибка удаления сессий:" << sessionQuery.lastError().text();
        return false;
    }

    return true;
}
bool Server::changePassword(const QString &token, const QString &userId,const QString& oldPassword, const QString& newPassword)
{
    QSqlQuery query;

    // 1. Проверка: существует ли пользователь с данным userId и совпадает ли старый пароль
    query.prepare("SELECT password FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса SELECT:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        QString storedPassword = query.value(0).toString();
        if (storedPassword != oldPassword) {
            qDebug() << "Неверный старый пароль для пользователя с ID:" << userId;
            return false;
        }
    } else {
        qDebug() << "Пользователь с ID" << userId << "не найден.";
        return false;
    }

    // 2. Обновление пароля в базе данных.
    // Можно зашифровать новый пароль, если это необходимо.
    QString encryptedNewPassword = cryptData.encryptQString(newPassword);
    query.prepare("UPDATE users SET password = :newPassword WHERE id = :id");
    query.bindValue(":newPassword", encryptedNewPassword);
    query.bindValue(":id", userId);

    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса UPDATE:" << query.lastError().text();
        return false;
    }

    // 3. Удаление всех сессий пользователя, кроме текущей (с переданным token).
    QSqlQuery sessionQuery;
    sessionQuery.prepare("DELETE FROM sessions "
                         "WHERE user_id = :user_id AND session_token != :current_token");
    sessionQuery.bindValue(":user_id", userId);
    sessionQuery.bindValue(":current_token", cryptData.encryptQString(token));

    if (!sessionQuery.exec()) {
        qDebug() << "Ошибка удаления сессий:" << sessionQuery.lastError().text();
        return false;
    }

    return true;
}
bool Server::deleteSession(const QString& token)
{
    QSqlQuery delQuery;
    delQuery.prepare("DELETE FROM sessions WHERE session_token = :token");
    delQuery.bindValue(":token", cryptData.encryptQString(token));
    if (!delQuery.exec()) {
        qDebug() << "Ошибка удаления сессии:" << delQuery.lastError().text();
        return false;
    }
    return true;
}
bool Server::isEmailRegistered(const QString &email)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM users WHERE email = :email");
    query.bindValue(":email", cryptData.encryptQString(email));
    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        return false; // либо можно трактовать как, что ошибка — email не найден, хотя лучше предусмотреть отдельную обработку
    }
    if (query.next() && query.value(0).toInt() > 0)
        return true;
    return false;
}
bool Server::findUserByEmail(const QString &email, QString &userId)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE email = :email");
    query.bindValue(":email", cryptData.encryptQString(email));

    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        userId = query.value(0).toString();
        return true;
    }

    return false;
}

// Регистрация нового пользователя
bool Server::registerNewUser(const QString &email, const QString &password, int &userId)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (email, password) VALUES (:email, :password)");
    query.bindValue(":email", cryptData.encryptQString(email));
    query.bindValue(":password", cryptData.encryptQString(password));
    if (!query.exec()) {
        qDebug() << "Ошибка регистрации пользователя:" << query.lastError().text();
        return false;
    }
    authorizationUser(email, password, userId);
    return true;
}
bool Server::authorizationUser(const QString &email, const QString &password, int &userId)
{
    QSqlQuery query;
    query.prepare("SELECT id, password FROM users WHERE email = :email");
    query.bindValue(":email", cryptData.encryptQString(email));

    if (!query.exec()) {
        qDebug() << "Error executing authorization query:" << query.lastError().text();
        return false;
    }

    // Если пользователь с заданным email не найден – авторизация неуспешна
    if (!query.next()) {
        return false;
    }

    QString storedEncryptedPassword = query.value("password").toString();
    QString inputEncryptedPassword = cryptData.encryptQString(password);

    if (storedEncryptedPassword == inputEncryptedPassword) {
        userId = query.value("id").toInt();
        return true;
    }
    return false;
}

QString Server::createSession(int userId)
{
    const int maxAttempts = 2000;
    const QDateTime expiresAt = QDateTime::currentDateTime().addMonths(1);

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        // Генерация нового токена
        QString sessionToken = cryptData.encryptQString(QUuid::createUuid().toString());

        QSqlQuery query;
        query.prepare("INSERT INTO sessions (session_token, user_id, expires_at, authorization_expires_at) "
                      "VALUES (:session_token, :user_id, :expires_at, :authorization_expires_at)");
        query.bindValue(":session_token", sessionToken);
        query.bindValue(":user_id", userId);
        query.bindValue(":expires_at", cryptData.encryptQString(expiresAt.toString(Qt::ISODate)));
        query.bindValue(":authorization_expires_at", cryptData.encryptQString(expiresAt.toString(Qt::ISODate)));

        if (query.exec()) {
            // Если токен успешно вставлен, возвращаем его
            qDebug() << "Сессия успешно создана";
            return sessionToken;
        } else {
            QSqlError err = query.lastError();
            // Если ошибка связана с дублированием (нарушение уникальности), повторяем попытку
            if (err.text().contains("UNIQUE", Qt::CaseInsensitive)) {
                qDebug() << "Дублирование токена" << cryptData.decryptQString(sessionToken) << ", попытка:" << attempt + 1;
                continue;
            }
            // Если ошибка другая, выводим сообщение об ошибке и выходим
            qDebug() << "Ошибка создания сессии:" << err.text();
            return "";
        }
    }
    qDebug() << "Не удалось создать уникальную сессию после" << maxAttempts << "попыток.";
    return "";
}
void Server::sendResponse(QTcpSocket* socket, const QJsonObject& response)
{
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QByteArray header;
    QDataStream ds(&header, QIODevice::WriteOnly);
    ds << quint32(data.size());

    socket->write(header + data);
}
