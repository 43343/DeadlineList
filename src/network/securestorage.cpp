#include "SecureStorage.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/kdf.h>
#include <openssl/sha.h>
#include <QDebug>
#include <QMessageAuthenticationCode>
#include <QProcess>
#include <QStringList>
#include <QDebug>


SecureStorage::SecureStorage(const QString &appName, QObject *parent)
    : QObject(parent)
{
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    m_storagePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (m_storagePath.isEmpty()) {
        m_storagePath = QDir::homePath() + "/." + appName;
    }
    m_storagePath += "/secure_data";

    ensureStorageDir();

    if (!initializeMasterKey()) {
        qCritical() << "Failed to initialize master key!";
    }
    m_key = getHardwareData();
}

SecureStorage::~SecureStorage()
{
    if (!m_masterKey.isEmpty()) {
        m_masterKey.fill(0);
    }
}

bool SecureStorage::ensureStorageDir() const
{
    QDir dir;
    if (!dir.mkpath(m_storagePath)) {
        qCritical() << "Failed to create storage directory:" << m_storagePath;
        return false;
    }

#ifndef Q_OS_WIN
    QFile::setPermissions(m_storagePath,
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif

    return true;
}

bool SecureStorage::initializeMasterKey()
{
    QString masterKeyPath = m_storagePath + "/.masterkey";
    QFile masterKeyFile(masterKeyPath);

    if (masterKeyFile.exists()) {
        if (masterKeyFile.open(QIODevice::ReadOnly)) {
            m_masterKey = masterKeyFile.readAll();
            masterKeyFile.close();
            return true;
        }
    }

    m_masterKey = generateRandomBytes(32); // AES-256 ключ
    if (m_masterKey.isEmpty()) {
        return false;
    }

    if (masterKeyFile.open(QIODevice::WriteOnly)) {
        masterKeyFile.write(m_masterKey);
        masterKeyFile.close();

#ifndef Q_OS_WIN
        QFile::setPermissions(masterKeyPath,
                              QFile::ReadOwner | QFile::WriteOwner);
#endif
        return true;
    }

    return false;
}

QByteArray SecureStorage::generateRandomBytes(int size) const
{
    QByteArray bytes(size, 0);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(bytes.data()), size) != 1) {
        qCritical() << "Failed to generate random bytes:" << ERR_error_string(ERR_get_error(), nullptr);
        return QByteArray();
    }
    return bytes;
}


QByteArray SecureStorage::encrypt(const QByteArray &data, const QByteArray &key) const
{
    if (data.isEmpty() || key.isEmpty()) {
        return QByteArray();
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qCritical() << "Failed to create cipher context";
        return QByteArray();
    }

    QByteArray iv = generateRandomBytes(16);
    if (iv.isEmpty()) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        qCritical() << "Encrypt init failed:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    int outLen = data.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc());
    QByteArray encrypted(outLen, 0);
    int finalLen = 0;

    if (EVP_EncryptUpdate(ctx,
                          reinterpret_cast<unsigned char*>(encrypted.data()), &outLen,
                          reinterpret_cast<const unsigned char*>(data.constData()), data.size()) != 1) {
        qCritical() << "Encrypt update failed:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (EVP_EncryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char*>(encrypted.data()) + outLen, &finalLen) != 1) {
        qCritical() << "Encrypt final failed:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    encrypted.resize(outLen + finalLen);
    EVP_CIPHER_CTX_free(ctx);

    return iv + encrypted;
}

QByteArray SecureStorage::decrypt(const QByteArray &encrypted, const QByteArray &key) const
{
    if (encrypted.size() <= 16 || key.isEmpty()) {
        return QByteArray();
    }

    QByteArray iv = encrypted.left(16);
    QByteArray ciphertext = encrypted.mid(16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qCritical() << "Failed to create cipher context";
        return QByteArray();
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        qCritical() << "Decrypt init failed:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    int outLen = ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc());
    QByteArray decrypted(outLen, 0);
    int finalLen = 0;

    if (EVP_DecryptUpdate(ctx,
                          reinterpret_cast<unsigned char*>(decrypted.data()), &outLen,
                          reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.size()) != 1) {
        qCritical() << "Decrypt update failed:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (EVP_DecryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char*>(decrypted.data()) + outLen, &finalLen) != 1) {
        qCritical() << "Decrypt final failed:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    decrypted.resize(outLen + finalLen);
    EVP_CIPHER_CTX_free(ctx);

    return decrypted;
}

bool SecureStorage::store(const QString &key, const QByteArray &data)
{
    QByteArray c_key = normalizeKey(key.toUtf8() + m_key.toUtf8());
    if (c_key.isEmpty() || m_masterKey.isEmpty()) {
        return false;
    }

    QByteArray encrypted = encrypt(data, c_key);
    if (encrypted.isEmpty()) {
        return false;
    }

    QString filePath = getFilePath(c_key);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QByteArray header("SECv1");
    QDataStream out(&file);
    out << header << encrypted;

    file.close();

#ifndef Q_OS_WIN
    QFile::setPermissions(filePath, QFile::ReadOwner | QFile::WriteOwner);
#endif

    return true;
}

QByteArray SecureStorage::load(const QString &key)
{
    QByteArray c_key = normalizeKey(key.toUtf8() + m_key.toUtf8());
    if (c_key.isEmpty() || m_masterKey.isEmpty()) {
        return QByteArray();
    }

    QString filePath = getFilePath(c_key);
    if (!QFile::exists(filePath)) {
        return QByteArray();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    QDataStream in(&file);
    QByteArray header, encrypted;
    in >> header >> encrypted;

    file.close();

    if (header != "SECv1") {
        return QByteArray();
    }

    return decrypt(encrypted, c_key);
}

bool SecureStorage::remove(const QString &key)
{
    QString c_key = normalizeKey(key.toUtf8() + m_key.toUtf8());
    if (c_key.isEmpty()) {
        return false;
    }

    QString filePath = getFilePath(c_key);
    if (!QFile::exists(filePath)) {
        return false;
    }

    secureFileDelete(filePath);
    return true;
}

bool SecureStorage::contains(const QString &key) const
{
    QString c_key = key + m_key;
    if (c_key.isEmpty()) {
        return false;
    }
    return QFile::exists(getFilePath(c_key));
}

void SecureStorage::clear()
{
    QDir dir(m_storagePath);
    foreach (const QString &file, dir.entryList(QDir::Files)) {
        if (file != ".masterkey") {
            secureFileDelete(dir.filePath(file));
        }
    }
}

QString SecureStorage::getFilePath(const QString &key) const
{
    QByteArray hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256);
    return QDir(m_storagePath).filePath(hash.toHex());
}

void SecureStorage::secureFileDelete(const QString &path) const
{
    if (!QFile::exists(path)) {
        return;
    }

#ifdef Q_OS_WIN
    QFile file(path);
    if (file.open(QIODevice::ReadWrite)) {
        QByteArray zeros(file.size(), 0);
        file.write(zeros);
        file.close();
    }
#else
    if (QProcess::execute("which shred") == 0) {
        QProcess::execute("shred", {"-u", "-z", "-n", "3", path});
        return;
    }

    QFile file(path);
    if (file.open(QIODevice::ReadWrite)) {
        QByteArray zeros(file.size(), 0);
        file.write(zeros);
        file.close();
    }
#endif

    QFile::remove(path);
}
QString SecureStorage::runCommand(const QString &command, const QStringList &arguments) const
{
    QProcess process;
    process.start(command, arguments);
    if (!process.waitForStarted())
        return QString();
    if (!process.waitForFinished())
        return QString();
    QString output = process.readAllStandardOutput();
    return output;
}
QString SecureStorage::getHardwareData() const
{
    QString output = "";
#ifdef Q_OS_WIN
    QString outputBaseBoardSerialNumber = runCommand("wmic", QStringList() << "baseboard" << "get" << "SerialNumber");
    QStringList linesBaseBoard = outputBaseBoardSerialNumber.split('\n', Qt::SkipEmptyParts);
    if (linesBaseBoard.size() >= 2) {
        output += linesBaseBoard[1].trimmed();
    }
    QString outputDiskSerialNumber = runCommand("wmic", QStringList() << "diskdrive" << "get" << "SerialNumber");
    QStringList linesDisk = outputDiskSerialNumber.split('\n', Qt::SkipEmptyParts);
    if (linesDisk.size() >= 2) {
        output += linesDisk[1].trimmed();
    }
#endif
#ifdef Q_OS_LINUX
    QFile file("/sys/devices/virtual/dmi/id/board_serial");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString outputBaseBoardSerialNumber = in.readLine().trimmed();
        file.close();
        output += outputBaseBoardSerialNumber;
    }
    QString outputDiskSerialNumber = runCommand("udevadm", QStringList() << "info" << "--query=all" << "--name=/dev/sda");
    // В выводе ищем строку, содержащую "ID_SERIAL="
    QStringList lines = outputDiskSerialNumber.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.contains("ID_SERIAL=")) {
            int pos = line.indexOf("ID_SERIAL=");
            if (pos != -1) {
                QString serial = line.mid(pos + QString("ID_SERIAL=").length()).trimmed();
                output += serial;
            }
        }
    }
#endif
    return output;
}
QByteArray SecureStorage::normalizeKey(const QByteArray &key) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(key.constData()), key.size(), hash);
    return QByteArray(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}
