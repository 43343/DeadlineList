#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include "config.h"

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(Config *config,QWidget *parent = nullptr);
    ~Settings();

private slots:
    void onLaunchByDefaultCheckBox();

private:
    Ui::Settings *ui;
    Config* m_config;

private slots:
    void save();
};

#endif // SETTINGS_H
