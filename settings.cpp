#include "settings.h"
#include "ui_settings.h"
#include "binarydatahandler.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

Settings::Settings(Config *config,QWidget *parent)
    : QDialog(parent)
    , currentConfig(config)
    , ui(new Ui::Settings)
{
    ui->setupUi(this);

    ui->enableTextNotificationsCheckBox->setChecked(config->enableTextNotifications);
    ui->enableSoundNotificationsCheckBox->setChecked(config->enableSoundNotifications);
    ui->launchByDefaultCheckbox->setChecked(config->launchByDefault);
    ui->launchingTrayCheckBox->setChecked(config->launchingTray);

    ui->launchingTrayCheckBox->setEnabled(ui->launchByDefaultCheckbox->isChecked());
    ui->launchingTrayLabel->setEnabled(ui->launchByDefaultCheckbox->isChecked());

    connect(ui->launchByDefaultCheckbox, &QCheckBox::clicked, this, &Settings::onLaunchByDefaultCheckBox);

    connect(ui->saveButton, &QPushButton::clicked, this, &Settings::save);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void Settings::onLaunchByDefaultCheckBox()
{
    QCheckBox* clickCheckBox = qobject_cast<QCheckBox*>(sender());
    ui->launchingTrayCheckBox->setEnabled(clickCheckBox->isChecked());
    ui->launchingTrayLabel->setEnabled(clickCheckBox->isChecked());
}

void Settings::save()
{
    currentConfig->enableTextNotifications = ui->enableTextNotificationsCheckBox->isChecked();
    currentConfig->enableSoundNotifications = ui->enableSoundNotificationsCheckBox->isChecked();
    currentConfig->launchByDefault = ui->launchByDefaultCheckbox->isChecked();
    currentConfig->launchingTray = ui->launchingTrayCheckBox->isChecked();
    overwritingFile("config", currentConfig);
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);
    if(ui->launchByDefaultCheckbox->isChecked())
    {
        QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        QString value = QString("\"%1\" --autostart").arg(appPath);
        settings.setValue("DeadlineList", value);
    }
    else
    {
        settings.remove("DeadlineList");
    }
#endif
#ifdef Q_OS_LINUX
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir autoStartDir(configPath + "/autostart");
    if (!autoStartDir.exists()) {
        // Создаем каталог, если он не существует
        if (!autoStartDir.mkpath(".")) {
            qWarning() << "Не удалось создать каталог для автозапуска:" << autoStartDir.absolutePath();
            return;
        }
    }
    QString filePath = autoStartDir.filePath("deadlinelist.desktop");
    if(ui->launchByDefaultCheckbox->isChecked())
    {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Невозможно открыть файл для записи:" << filePath;
            return;
        }
        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        // Указываем команду запуска, включающую параметр --autostart
        out << "Exec=" << QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "--autostart" << "\n";
        out << "Hidden=false\n";
        out << "NoDisplay=false\n";
        out << "X-GNOME-Autostart-enabled=true\n";
        out << "Name=DeadlineList\n";
        out << "Comment=Автозапуск DeadlineList с параметром --autostart\n";
        file.close();
        qDebug() << "Добавлен ярлык автозапуска (Linux):" << filePath;
    }
    else
    {
        if (QFile::exists(filePath)) {
            if (QFile::remove(filePath)) {
                qDebug() << "Удалён ярлык автозапуска (Linux):" << filePath;
            } else {
                qWarning() << "Не удалось удалить файл:" << filePath;
            }
        } else {
            qDebug() << "Файл автозапуска не найден:" << filePath;
        }
    }
#endif
    accept();
}

Settings::~Settings()
{
    delete ui;
}
