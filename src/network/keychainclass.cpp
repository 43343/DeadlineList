#include "keychainclass.h"
#include <QDebug>
#include <QProcess>
#include <QStringList>
#include <QString>

KeyChainClass::KeyChainClass(QObject *parent)
    : QObject(parent)
{
}

void KeyChainClass::readToken()
{
    QKeychain::ReadPasswordJob* m_readCredentialTokenJob = new QKeychain::ReadPasswordJob("dedlinelist", this);
    m_readCredentialTokenJob->setKey(getHardwareData() + "token");

    connect(m_readCredentialTokenJob, &QKeychain::Job::finished, this, [=]{
        if (m_readCredentialTokenJob->error()) {
            emit error(
                tr("Read key failed: %1").arg(qPrintable(m_readCredentialTokenJob->errorString())));
            return;
        }
        emit tokenRestored(m_readCredentialTokenJob->textData());
        m_readCredentialTokenJob->deleteLater();
    });

    m_readCredentialTokenJob->start();
}

void KeyChainClass::writeToken(const QString &value)
{
    QKeychain::WritePasswordJob* m_writeCredentialTokenJob = new QKeychain::WritePasswordJob("dedlinelist", this);
    m_writeCredentialTokenJob->setKey(getHardwareData() + "token");
    m_writeCredentialTokenJob->setTextData(value);

    connect(m_writeCredentialTokenJob, &QKeychain::Job::finished, this, [=]{
        if (m_writeCredentialTokenJob->error()) {
            emit error(
                tr("Read key failed: %1").arg(qPrintable(m_writeCredentialTokenJob->errorString())));
            return;
        }
        emit tokenStored();
        m_writeCredentialTokenJob->deleteLater();
    });
    m_writeCredentialTokenJob->start();
}
void KeyChainClass::readUserId()
{
    QKeychain::ReadPasswordJob* m_readCredentialUserIdJob = new QKeychain::ReadPasswordJob("dedlinelist", this);
    m_readCredentialUserIdJob->setKey(getHardwareData() + "userid");

    connect(m_readCredentialUserIdJob, &QKeychain::Job::finished, this, [=]{
        if (m_readCredentialUserIdJob->error()) {
            emit error(
                tr("Read key failed: %1").arg(qPrintable(m_readCredentialUserIdJob->errorString())));
            return;
        }
        emit userIdRestored(m_readCredentialUserIdJob->textData());
       m_readCredentialUserIdJob->deleteLater();
    });

    m_readCredentialUserIdJob->start();
}

void KeyChainClass::writeUserId(const QString &value)
{
    QKeychain::WritePasswordJob* m_writeCredentialUserIdJob = new QKeychain::WritePasswordJob("dedlinelist", this);
    m_writeCredentialUserIdJob->setKey(getHardwareData() + "userid");
   m_writeCredentialUserIdJob->setTextData(value);

    connect(m_writeCredentialUserIdJob, &QKeychain::Job::finished, this, [=]{
        if (m_writeCredentialUserIdJob->error()) {
            emit error(
                tr("Read key failed: %1").arg(qPrintable(m_writeCredentialUserIdJob->errorString())));
            return;
        }
        emit userIdStored();
        m_writeCredentialUserIdJob->deleteLater();
    });
    m_writeCredentialUserIdJob->start();
}
#ifdef Q_OS_WIN
QString KeyChainClass::runCommand(const QString &command, const QStringList &arguments) const
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
#endif
QString KeyChainClass::getHardwareData() const
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
    QStringList lines = outputDiskSerialNumber.split('\n', QString::SkipEmptyParts);
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
