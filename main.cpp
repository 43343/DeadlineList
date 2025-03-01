#include "mainwindow.h"

#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    const QString sharedMemoryKey = "MyDeadlineAppKeyYCDCHGgcchfcDCXfdxHFCgxFXszBGhgfdcDXfnhfgdx";
    QSharedMemory sharedMemory(sharedMemoryKey);
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, "Приложение уже запущено",
                             "Приложение уже запущено. Закройте его перед повторным запуском.");
        return 0;
    }

    MainWindow w;
    return a.exec();
}
