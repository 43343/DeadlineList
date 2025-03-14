#ifndef CRYPTDATA_H
#define CRYPTDATA_H
#include <QString>

class CryptData
{
public:
    CryptData();
    QString encryptQString(const QString &text) const;
    QString decryptQString(const QString &encryptedText) const;
private:
#ifdef Q_OS_WIN
    QString runCommand(const QString &command, const QStringList &arguments) const;
#endif
    QString getHardwareData() const;
};

#endif // CRYPTDATA_H
