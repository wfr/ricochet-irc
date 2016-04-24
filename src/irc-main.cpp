/* Ricochet - https://ricochet.im/
 * Copyright (C) 2014, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/IdentityManager.h"
#include "tor/TorManager.h"
#include "tor/TorControl.h"
#include "tor/TorProcess.h"
#include "utils/CryptoKey.h"
#include "utils/SecureRNG.h"
#include "utils/Settings.h"
#include <QCoreApplication>
#include "irc/RicochetIrcServer.h"
#include <QLibraryInfo>
#include <QSettings>
#include <QTime>
#include <QDir>
#include <QTranslator>
#include <QLocale>
#include <QLockFile>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <openssl/crypto.h>
#include <QMetaObject>
#include <QDebug>
#include <QtCore>
#include <iostream>

static bool initSettings(SettingsFile *settings, QLockFile **lockFile, QString &errorMessage);
static bool importLegacySettings(SettingsFile *settings, const QString &oldPath);
static void initTranslation();

class Task : public QObject
{
    Q_OBJECT
public:
    Task(QCoreApplication *app) : QObject(app) { }

private:

signals:
    void finished();

public slots:
    void run()
    {
        bool result;
        SettingsObject settings;
        uint16_t port = settings.read("irc.port", 6667).toInt();
        QString password = settings.read("irc.password", QStringLiteral("")).toString();

        RicochetIrcServer *irc_server = new RicochetIrcServer(this, port, password);
        QMetaObject::invokeMethod(irc_server, "run", Q_RETURN_ARG(bool, result));
        if(!result)
        {
            qDebug() << "!result";
            emit finished();
        }

        std::cout << std::endl;
        std::cout << "## IRC server started:" << std::endl;
        std::cout << "Host/port: 127.0.0.1/" << port << std::endl;
        std::cout << "Password:  " << password.toUtf8().data() << std::endl;
        std::cout << std::endl;
        std::cout << "### WeeChat client setup:" << std::endl;
        std::cout << "/server add ricochet 127.0.0.1/" << port << std::endl;
        std::cout << "/set irc.server.ricochet.password \"" << password.toUtf8().data() << "\"" << std::endl;
        std::cout << "/set logger.level.irc.ricochet 0" << std::endl;
        std::cout << std::endl;
        std::cout.flush();
    }
};

#include "irc-main.moc"

int main(int argc, char *argv[])
{
    /* Disable rwx memory.
       This will also ensure full PAX/Grsecurity protections. */
    qputenv("QV4_FORCE_INTERPRETER",  "1");
    qputenv("QT_ENABLE_REGEXP_JIT",   "0");

    QCoreApplication a(argc, argv);
    a.setApplicationVersion(QLatin1String("1.1.2"));
    a.setOrganizationName(QStringLiteral("Ricochet"));
    qSetMessagePattern(QString::fromLatin1("%{file}(%{line}): %{message}"));

    QScopedPointer<SettingsFile> settings(new SettingsFile);
    SettingsObject::setDefaultFile(settings.data());

    QString error;
    QLockFile *lock = 0;
    if (!initSettings(settings.data(), &lock, error)) {
        qCritical() << error;
        return 1;
    }
    QScopedPointer<QLockFile> lockFile(lock);

    initTranslation();

    /* Initialize OpenSSL's allocator */
    CRYPTO_malloc_init();

    /* Seed the OpenSSL RNG */
    if (!SecureRNG::seed())
        qFatal("Failed to initialize RNG");
    qsrand(SecureRNG::randomInt(UINT_MAX));

    /* Tor control manager */
    Tor::TorManager *torManager = Tor::TorManager::instance();
    torManager->setDataDirectory(QFileInfo(settings->filePath()).path() + QStringLiteral("/tor/"));
    torControl = torManager->control();
    //torManager->start();

    /* Identities */
    identityManager = new IdentityManager;
    QScopedPointer<IdentityManager> scopedIdentityManager(identityManager);

    Task *task = new Task(&a);
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));
    QMetaObject::invokeMethod( task, "run", Qt::QueuedConnection );

    return a.exec();
}

static QString userConfigPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString oldPath = path;
    oldPath.replace(QStringLiteral("Ricochet"), QStringLiteral("Torsion"), Qt::CaseInsensitive);
    if (QFile::exists(oldPath))
        return oldPath;
    return path;
}

#ifdef Q_OS_MAC
static QString appBundlePath()
{
    QString path = QApplication::applicationDirPath();
    int p = path.lastIndexOf(QLatin1String(".app/"));
    if (p >= 0)
    {
        p = path.lastIndexOf(QLatin1Char('/'), p);
        path = path.left(p+1);
    }

    return path;
}
#endif

// Writes default settings to settings object. Does not care about any
// preexisting values, therefore this is best used on a fresh object.
static void loadDefaultSettings(SettingsFile *settings)
{
    settings->root()->write("ui.combinedChatWindow", true);
}

static bool initSettings(SettingsFile *settings, QLockFile **lockFile, QString &errorMessage)
{
    /* If built in portable mode (default), configuration is stored in the 'config'
     * directory next to the binary. If not writable, launching fails.
     *
     * Portable OS X is an exception. In that case, configuration is stored in a
     * 'config.ricochet' folder next to the application bundle, unless the application
     * path contains "/Applications", in which case non-portable mode is used.
     *
     * When not in portable mode, a platform-specific per-user config location is used.
     *
     * This behavior may be overriden by passing a folder path as the first argument.
     */

    QString configPath;

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main","Anonymous peer-to-peer instant messaging, IRC gateway"));
    QCommandLineOption opt_config_path(QStringLiteral("config"),
                                       QCoreApplication::translate("main", "Configuration directory."),
                                       QStringLiteral("config-path"));
    parser.addOption(opt_config_path);
    QCommandLineOption opt_irc_port(QStringLiteral("port"),
                                    QCoreApplication::translate("irc", "Set IRC server port."),
                                    QStringLiteral("port"),
                                    QStringLiteral("6667"));
    parser.addOption(opt_irc_port);
    QCommandLineOption opt_irc_password(QStringLiteral("generate-password"),
                                        QCoreApplication::translate("irc", "Generate random IRC password."));
    parser.addOption(opt_irc_password);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(qApp->arguments());
    const QStringList args = parser.positionalArguments();
    if(args.count() > 0) {
        parser.showHelp(1);
    }


    if(parser.isSet(opt_config_path)) {
        configPath = parser.value(opt_config_path);
    } else {
#ifndef RICOCHET_NO_PORTABLE
# ifdef Q_OS_MAC
        if (!qApp->applicationDirPath().contains(QStringLiteral("/Applications"))) {
            // Try old configuration path first
            configPath = appBundlePath() + QStringLiteral("config.torsion");
            if (!QFile::exists(configPath))
                configPath = appBundlePath() + QStringLiteral("config.ricochet");
        }
# else
        configPath = qApp->applicationDirPath() + QStringLiteral("/config");
# endif
#endif
        if (configPath.isEmpty())
            configPath = userConfigPath();
    }

    QDir dir(configPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        errorMessage = QStringLiteral("Cannot create directory: %1").arg(dir.path());
        return false;
    }

    // Reset to config directory for consistency; avoid depending on this behavior for paths
    if (QDir::setCurrent(dir.absolutePath()) && dir.isRelative())
        dir.setPath(QStringLiteral("."));

    QLockFile *lock = new QLockFile(dir.filePath(QStringLiteral("ricochet.json.lock")));
    *lockFile = lock;
    lock->setStaleLockTime(0);
    if (!lock->tryLock()) {
        if (lock->error() == QLockFile::LockFailedError) {
            // This happens if a stale lock file exists and another process uses that PID.
            // Try removing the stale file, which will fail if a real process is holding a
            // file-level lock. A false error is more problematic than not locking properly
            // on corner-case systems.
            if (!lock->removeStaleLockFile() || !lock->tryLock()) {
                errorMessage = QStringLiteral("Configuration file is already in use");
                return false;
            } else
                qDebug() << "Removed stale lock file";
        } else {
            errorMessage = QStringLiteral("Cannot write configuration file (failed to acquire lock)");
            return false;
        }
    }

    settings->setFilePath(dir.filePath(QStringLiteral("ricochet.json")));
    if (settings->hasError()) {
        errorMessage = settings->errorMessage();
        return false;
    }

    if (settings->root()->data().isEmpty()) {
        QString filePath = dir.filePath(QStringLiteral("Torsion.ini"));
        if (!QFile::exists(filePath))
            filePath = dir.filePath(QStringLiteral("ricochet.ini"));
        if (QFile::exists(filePath))
            importLegacySettings(settings, filePath);
    }
    // if still empty, load defaults here
    if (settings->root()->data().isEmpty()) {
        loadDefaultSettings(settings);
    }

    if(parser.isSet(opt_irc_port)) {
        bool ok;
        int port = parser.value(QStringLiteral("port")).toInt(&ok);
        if(!ok)
        {
            errorMessage = QCoreApplication::translate("irc", "invalid IRC server port");
            return false;
        }
        settings->root()->write("irc.port", port);
        qDebug() << "IRC server port is" << port;
    }

    if(parser.isSet(opt_irc_password)) {
        settings->root()->write("irc.password", QString::fromLatin1(SecureRNG::random(18).toBase64()));
    }

    return true;
}

static void copyKeys(QSettings &old, SettingsObject *object)
{
    foreach (const QString &key, old.childKeys()) {
        QVariant value = old.value(key);
        if ((QMetaType::Type)value.type() == QMetaType::QDateTime)
            object->write(key, value.toDateTime());
        else if ((QMetaType::Type)value.type() == QMetaType::QByteArray)
            object->write(key, Base64Encode(value.toByteArray()));
        else
            object->write(key, value.toString());
    }
}

static bool importLegacySettings(SettingsFile *settings, const QString &oldPath)
{
    QSettings old(oldPath, QSettings::IniFormat);
    SettingsObject *root = settings->root();
    QVariant value;

    qDebug() << "Importing legacy format settings from" << oldPath;

    if (!(value = old.value(QStringLiteral("tor/controlIp"))).isNull())
        root->write("tor.controlAddress", value.toString());
    if (!(value = old.value(QStringLiteral("tor/controlPort"))).isNull())
        root->write("tor.controlPort", value.toInt());
    if (!(value = old.value(QStringLiteral("tor/authPassword"))).isNull())
        root->write("tor.controlPassword", value.toString());
    if (!(value = old.value(QStringLiteral("tor/socksIp"))).isNull())
        root->write("tor.socksAddress", value.toString());
    if (!(value = old.value(QStringLiteral("tor/socksPort"))).isNull())
        root->write("tor.socksPort", value.toInt());
    if (!(value = old.value(QStringLiteral("tor/executablePath"))).isNull())
        root->write("tor.executablePath", value.toString());
    if (!(value = old.value(QStringLiteral("core/neverPublishService"))).isNull())
        root->write("tor.neverPublishServices", value.toBool());
    if (!(value = old.value(QStringLiteral("identity/0/dataDirectory"))).isNull())
        root->write("identity.dataDirectory", value.toString());
    if (!(value = old.value(QStringLiteral("identity/0/createNewService"))).isNull())
        root->write("identity.initializing", value.toBool());
    if (!(value = old.value(QStringLiteral("core/listenIp"))).isNull())
        root->write("identity.localListenAddress", value.toString());
    if (!(value = old.value(QStringLiteral("core/listenPort"))).isNull())
        root->write("identity.localListenPort", value.toInt());

    {
        old.beginGroup(QStringLiteral("contacts"));
        QStringList ids = old.childGroups();
        foreach (const QString &id, ids) {
            old.beginGroup(id);
            SettingsObject userObject(root, QStringLiteral("contacts.%1").arg(id));

            copyKeys(old, &userObject);

            if (old.childGroups().contains(QStringLiteral("request"))) {
                old.beginGroup(QStringLiteral("request"));
                QStringList requestKeys = old.childKeys();
                foreach (const QString &key, requestKeys)
                    userObject.write(QStringLiteral("request.") + key, old.value(key).toString());
                old.endGroup();
            }

            old.endGroup();
        }
        old.endGroup();
    }

    {
        old.beginGroup(QStringLiteral("contactRequests"));
        QStringList contacts = old.childGroups();

        foreach (const QString &hostname, contacts) {
            old.beginGroup(hostname);
            SettingsObject requestObject(root, QStringLiteral("contactRequests.%1").arg(hostname));
            copyKeys(old, &requestObject);
            old.endGroup();
        }

        old.endGroup();
    }

    if (!(value = old.value(QStringLiteral("core/hostnameBlacklist"))).isNull()) {
        QStringList blacklist = value.toStringList();
        root->write("identity.hostnameBlacklist", QJsonArray::fromStringList(blacklist));
    }

    return true;
}

static void initTranslation()
{
    QTranslator *translator = new QTranslator;

    bool ok = false;
    QString appPath = qApp->applicationDirPath();
    QString resPath = QLatin1String(":/lang/");

    QLocale locale = QLocale::system();
    if (!qgetenv("RICOCHET_LOCALE").isEmpty()) {
        locale = QLocale(QString::fromLatin1(qgetenv("RICOCHET_LOCALE")));
        qDebug() << "Forcing locale" << locale << "from environment" << locale.uiLanguages();
    }

    SettingsObject settings;
    QString settingsLanguage(settings.read("ui.language").toString());

    if (!settingsLanguage.isEmpty()) {
        locale = settingsLanguage;
    } else {
        //write an empty string to get "System default" language selected automatically in preferences
        settings.write(QStringLiteral("ui.language"), QString());
    }

    ok = translator->load(locale, QStringLiteral("ricochet"), QStringLiteral("_"), appPath);
    if (!ok)
        ok = translator->load(locale, QStringLiteral("ricochet"), QStringLiteral("_"), resPath);

    if (ok) {
        qApp->installTranslator(translator);

        QTranslator *qtTranslator = new QTranslator;
        ok = qtTranslator->load(QStringLiteral("qt_") + locale.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
        if (ok)
            qApp->installTranslator(qtTranslator);
        else
            delete qtTranslator;
    } else
        delete translator;
}

