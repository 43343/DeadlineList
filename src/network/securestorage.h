#ifndef SECURESTORAGE_H
#define SECURESTORAGE_H
#include <QObject>
#include <QString>
#include <QByteArray>


class SecureStorage : public QObject
{
    Q_OBJECT
public:
    explicit SecureStorage(const QString &appName, QObject *parent = nullptr);
    ~SecureStorage();

    bool store(const QString &key, const QByteArray &data);
    QByteArray load(const QString &key);
    bool remove(const QString &key);
    bool contains(const QString &key) const;
    void clear();

private:
    QString m_storagePath;
    QByteArray m_masterKey;
    QString m_key;

    QString getFilePath(const QString &key) const;
    bool ensureStorageDir() const;
    QByteArray encrypt(const QByteArray &data, const QByteArray &key) const;
    QByteArray decrypt(const QByteArray &encrypted, const QByteArray &key) const;
    void secureFileDelete(const QString &path) const;
    bool initializeMasterKey();
    QByteArray generateRandomBytes(int size) const;
    QString runCommand(const QString &command, const QStringList &arguments) const;
    QString getHardwareData() const;
    QByteArray normalizeKey(const QByteArray &key) const;
};

#endif // SECURESTORAGE_H
