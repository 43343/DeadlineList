#ifndef BINARYDATAHANDLER_H
#define BINARYDATAHANDLER_H
#include <QString>
#include <QList>
#include "taskform.h"
#include "config.h"

bool overwritingFile(const QString &fileName, const Config* config);
bool loadFromFile(const QString &fileName, Config* config);

bool overwritingFile(const QString &fileName, const QList<TaskForm*>* taskList);
bool loadFromFile(const QString &fileName, QList<TaskForm*>* taskList, QWidget *parent);

QString fullFilePath(const QString &fileName);

#endif // BINARYDATAHANDLER_H
