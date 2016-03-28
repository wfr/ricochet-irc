#ifndef MAINSHARED_H
#define MAINSHARED_H

#include "utils/Settings.h"
#include <QSettings>
#include <QLockFile>

namespace Application {
    bool initSettings(SettingsFile *settings, QLockFile **lockFile, QString &errorMessage);
    bool importLegacySettings(SettingsFile *settings, const QString &oldPath);
    void initTranslation();

    QString userConfigPath();
#ifdef Q_OS_MAC
    static QString appBundlePath();
#endif
    void copyKeys(QSettings &old, SettingsObject *object);
    bool importLegacySettings(SettingsFile *settings, const QString &oldPath);
    void initTranslation();
}

#endif // MAINSHARED_H

