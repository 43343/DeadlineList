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
    QString runCommand(const QString &command, const QStringList &arguments) const;
    QString getHardwareData() const;
};

#endif // CRYPTDATA_H
