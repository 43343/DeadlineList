#ifndef CONFIG_H
#define CONFIG_H
#include <QString>

class Config
{
public:
    Config() {};
    bool enableTextNotifications = true;
    bool enableSoundNotifications = true;
    bool launchByDefault = false;
    bool launchingTray = false;
    QString language = "";
};

#endif // CONFIG_H

