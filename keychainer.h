#ifndef KEYCHAINER_H
#define KEYCHAINER_H
#include <QString>
#include <qtkeychain/keychain.h>

Q_INVOKABLE void readKey(const QString &key);
Q_INVOKABLE void writeKey(const QString &key, const QString &value);

extern QKeychain::ReadPasswordJob readCredentialJob;
extern QKeychain::WritePasswordJob writeCredentialJob;

#endif // KEYCHAINER_H
