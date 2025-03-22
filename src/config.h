#ifndef CONFIG_H
#define CONFIG_H

class Config
{
public:
    Config() {};
    bool enableTextNotifications = true;
    bool enableSoundNotifications = true;
    bool launchByDefault = false;
    bool launchingTray = false;
};

#endif // CONFIG_H

