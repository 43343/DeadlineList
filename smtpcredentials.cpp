#include "smtpcredentials.h"
#include <QDebug>
void loadSmtpCredentials(QString &email, QString &password)
{
    QFile file("smtp.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл smtp.txt:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("Email=", Qt::CaseInsensitive)) {
            email = line.section('=', 1).trimmed();
        }

        if (line.startsWith("Password=", Qt::CaseInsensitive)) {
            password = line.section('=', 1).trimmed();
        }
        qDebug() << "Email" << email << "Password" << password;
    }

    file.close();
}
