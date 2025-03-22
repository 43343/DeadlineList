#ifndef KEYCHAINCLASS_H
#define KEYCHAINCLASS_H
#include <QObject>
#include <qtkeychain/keychain.h>

class KeyChainClass : public QObject
{
    Q_OBJECT
public:
    KeyChainClass(QObject *parent = nullptr);

    Q_INVOKABLE void readToken();
    Q_INVOKABLE void writeToken(const QString &value);
    Q_INVOKABLE void readUserId();
    Q_INVOKABLE void writeUserId(const QString &value);

signals:
    void tokenStored();
    void tokenRestored(const QString &value);
    void userIdStored();
    void userIdRestored(const QString &value);
    void error(const QString &errorText);
private:
    QString getHardwareData() const;
    QString runCommand(const QString &command, const QStringList &arguments) const;
};

#endif // KEYCHAINCLASS_H
