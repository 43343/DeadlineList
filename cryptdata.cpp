#include "cryptdata.h"
#include <QProcess>
#include <QDebug>

CryptData::CryptData() {}

QString CryptData::encryptQString(const QString &text) const{
    // Преобразуем исходную строку в UTF-8 байты
    QByteArray data = text.toUtf8();
    QByteArray keyData = getHardwareData().toUtf8();
    QByteArray result;

    // Выполняем XOR для каждого байта данных с соответствующим байтом ключа
    for (int i = 0; i < data.size(); ++i) {
        char encryptedChar = data.at(i) ^ keyData.at(i % keyData.size());
        result.append(encryptedChar);
    }

    // Кодируем результат в Base64 – это позволит передавать его в виде строки
    return QString::fromUtf8(result.toBase64());
}
QString CryptData::decryptQString(const QString &encryptedText) const {
    // Декодируем Base64 в исходный зашифрованный байтовый массив
    QByteArray encryptedData = QByteArray::fromBase64(encryptedText.toUtf8());
    QByteArray keyData = getHardwareData().toUtf8();
    QByteArray result;

    // Выполняем XOR для восстановления исходных байт
    for (int i = 0; i < encryptedData.size(); ++i) {
        char decryptedChar = encryptedData.at(i) ^ keyData.at(i % keyData.size());
        result.append(decryptedChar);
    }

    // Преобразуем результат обратно в QString с использованием кодировки UTF-8
    return QString::fromUtf8(result);
}
#ifdef Q_OS_WIN
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
#endif
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
