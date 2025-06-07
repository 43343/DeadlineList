#include "cryptdata.h"
#include <QProcess>
#include <QDebug>
#ifdef Q_OS_LINUX
#include <QFile>
#endif

CryptData::CryptData()
{
    m_hardwareData = getHardwareData();
}

QString CryptData::encryptQString(const QString &text) const{
    QByteArray data = text.toUtf8();
    QByteArray keyData = m_hardwareData.toUtf8();
    QByteArray result;

    for (int i = 0; i < data.size(); ++i) {
        char encryptedChar = data.at(i) ^ keyData.at(i % keyData.size());
        result.append(encryptedChar);
    }

    return QString::fromUtf8(result.toBase64());
}
QString CryptData::decryptQString(const QString &encryptedText) const {
    QByteArray encryptedData = QByteArray::fromBase64(encryptedText.toUtf8());
    QByteArray keyData = m_hardwareData.toUtf8();
    QByteArray result;

    for (int i = 0; i < encryptedData.size(); ++i) {
        char decryptedChar = encryptedData.at(i) ^ keyData.at(i % keyData.size());
        result.append(decryptedChar);
    }

    return QString::fromUtf8(result);
}
QString CryptData::runCommand(const QString &command, const QStringList &arguments) const
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
QString CryptData::getHardwareData() const
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
