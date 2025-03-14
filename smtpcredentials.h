#ifndef SMTPCREDENTIALS_H
#define SMTPCREDENTIALS_H
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>

void loadSmtpCredentials(QString &email, QString &password);

#endif // SMTPCREDENTIALS_H
