/* Ricochet-IRC - https://github.com/wfr/ricochet-irc/
 * Wolfgang Frisch <wfrisch@riseup.net>
 *
 * Derived from:
 * Ricochet Refresh - https://ricochetrefresh.net/
 * Copyright (C) 2019, Blueprint For Free Speech <ricochet@blueprintforfreespeech.net>
 *
 * Ricochet - https://ricochet.im/
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

#include "utils/Settings.h"

#include <libtego_callbacks.hpp>

// shim replacements
#include "shims/TorControl.h"
#include "shims/TorManager.h"
#include "shims/UserIdentity.h"
#include "RicochetIrcServer.h"
#include "RicochetIrcServerTask.h"

#include <openssl/crypto.h>
#include <iostream>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QLockFile>
#include <QMetaObject>
#include <QSettings>
#include <QStandardPaths>
#include <QtCore>
#include <QTime>
#include <QTranslator>

static bool initSettings(SettingsFile *settings, QLockFile **lockFile, QString &errorMessage);
static void initTranslation();
static QString randomPassword(size_t len = 14);

int main(int argc, char *argv[]) try
{
    /* Disable rwx memory.
       This will also ensure full PAX/Grsecurity protections. */
    qputenv("QV4_FORCE_INTERPRETER",  "1");
    qputenv("QT_ENABLE_REGEXP_JIT",   "0");

    QCoreApplication a(argc, argv);

    tego_context_t* tegoContext = nullptr;
    tego_initialize(&tegoContext, tego::throw_on_error());

    auto tego_cleanup = tego::make_scope_exit([=]() -> void {
        tego_uninitialize(tegoContext, tego::throw_on_error());
    });

    init_libtego_callbacks(tegoContext);

    a.setApplicationVersion(QLatin1String(TEGO_VERSION_STR));
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

    // init our tor shims
    shims::TorControl::torControl = new shims::TorControl(tegoContext);
    shims::TorManager::torManager = new shims::TorManager(tegoContext);

    // start Tor
    {
        std::unique_ptr<tego_tor_launch_config_t> launchConfig;
        tego_tor_launch_config_initialize(tego::out(launchConfig), tego::throw_on_error());

        auto rawFilePath = (QFileInfo(settings->filePath()).path() + QStringLiteral("/tor/")).toUtf8();
        tego_tor_launch_config_set_data_directory(
            launchConfig.get(),
            rawFilePath.data(),
            rawFilePath.size(),
            tego::throw_on_error());

        tego_context_start_tor(tegoContext, launchConfig.get(), tego::throw_on_error());
    }

    /* Identities */

    // init our shims
    shims::UserIdentity::userIdentity = new shims::UserIdentity(tegoContext);
    auto contactsManager = shims::UserIdentity::userIdentity->getContacts();

    auto privateKeyString = SettingsObject("identity").read<QString>("privateKey");
    if (privateKeyString.isEmpty())
    {
        tego_context_start_service(
            tegoContext,
            nullptr,
            nullptr,
            nullptr,
            0,
            tego::throw_on_error());
    }
    else
    {
        // construct privatekey from privateKey keyblob
        std::unique_ptr<tego_ed25519_private_key_t> privateKey;
        auto keyBlob = privateKeyString.toUtf8();

        tego_ed25519_private_key_from_ed25519_keyblob(
            tego::out(privateKey),
            keyBlob.data(),
            keyBlob.size(),
            tego::throw_on_error());

        // load all of our user objects
        std::vector<tego_user_id_t*> userIds;
        std::vector<tego_user_type_t> userTypes;
        auto userIdCleanup = tego::make_scope_exit([&]() -> void
        {
            std::for_each(userIds.begin(), userIds.end(), &tego_user_id_delete);
        });

        // map strings saved in json with tego types
        const static QMap<QString, tego_user_type_t> stringToUserType =
        {
            {QString("allowed"), tego_user_type_allowed},
            {QString("requesting"), tego_user_type_requesting},
            {QString("blocked"), tego_user_type_blocked},
            {QString("pending"), tego_user_type_pending},
            {QString("rejected"), tego_user_type_rejected},
        };

        auto usersJson = SettingsObject("users").data();
        for(auto it = usersJson.begin(); it != usersJson.end(); ++it)
        {
            // get the user's service id
            const auto serviceIdString = it.key();
            const auto serviceIdRaw = serviceIdString.toUtf8();

            std::unique_ptr<tego_v3_onion_service_id_t> serviceId;
            tego_v3_onion_service_id_from_string(
                tego::out(serviceId),
                serviceIdRaw.data(),
                serviceIdRaw.size(),
                tego::throw_on_error());

            std::unique_ptr<tego_user_id_t> userId;
            tego_user_id_from_v3_onion_service_id(
                tego::out(userId),
                serviceId.get(),
                tego::throw_on_error());
            userIds.push_back(userId.release());

            // load relevant data
            const auto& userData = it.value().toObject();
            auto typeString = userData.value("type").toString();

            Q_ASSERT(stringToUserType.contains(typeString));
            auto type = stringToUserType.value(typeString);
            userTypes.push_back(type);

            if (type == tego_user_type_allowed ||
                type == tego_user_type_pending ||
                type == tego_user_type_rejected)
            {
                const auto nickname = userData.value("nickname").toString();
                auto contact = contactsManager->addContact(serviceIdString, nickname);
                switch(type)
                {
                case tego_user_type_allowed:
                    contact->setStatus(shims::ContactUser::Offline);
                    break;
                case tego_user_type_pending:
                    contact->setStatus(shims::ContactUser::RequestPending);
                    break;
                case tego_user_type_rejected:
                    contact->setStatus(shims::ContactUser::RequestRejected);
                    break;
                default:
                    break;
                }
            }
        }
        Q_ASSERT(userIds.size() == userTypes.size());
        const size_t userCount = userIds.size();

        tego_context_start_service(
            tegoContext,
            privateKey.get(),
            userIds.data(),
            userTypes.data(),
            userCount,
            tego::throw_on_error());
    }

    // Start the IRC server
    auto task = new RicochetIrcServerTask(&a);
    QObject::connect(task, &RicochetIrcServerTask::finished, &a, QCoreApplication::quit);

    QMetaObject::invokeMethod( task, "run", Qt::QueuedConnection );

    return a.exec();
}
catch(std::exception& re)
{
    qDebug() << "Caught Exception: " << re.what();
    return -1;
}

#ifdef Q_OS_MAC
// returns the directory to place the config.ricochet directory on macOS
// no trailing '/'
static QString appBundlePath()
{
    QString path = QApplication::applicationDirPath();
    // if user left the binaries insidie the app bundle
    int p = path.lastIndexOf(QLatin1String(".app/"));
    if (p >= 0)
    {
        // just some binaries floating around somewhere
        p = path.lastIndexOf(QLatin1Char('/'), p);
        path = path.left(p);
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
    /* ricochet-refresh by default loads and saves configuration files from QStandardPaths::AppLocalDataLocation
     *
     * Linux: ~/.local/share/ricochet-refresh
     * Windows: C:/Users/<USER>/AppData/Local/ricochet-refresh
     * macOS: "~/Library/Application Support/ricochet-refresh"
     *
     * ricochet-refresh can also load configuration files from a custom directory passed in as the first argument
     */


    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main","Anonymous peer-to-peer instant messaging, IRC gateway"));
    QCommandLineOption opt_config_path(QStringLiteral("config"),
                                       QCoreApplication::translate("main", "Select configuration directory."),
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

    QString configPath;
    const QStringList args = parser.positionalArguments();
    if(args.count() > 0) {
        parser.showHelp(1);
    }


    if(parser.isSet(opt_config_path)) {
        configPath = parser.value(opt_config_path);
    } else {
        // TODO: remove this profile migration after sufficient time has passed (EOY 2021)
        auto legacyConfigPath = []() -> QString {
            QString configPath;
#ifdef Q_OS_MAC
            // if the user has installed it to /Applications
            if (qApp->applicationDirPath().contains(QStringLiteral("/Applications"))) {
                // ~Library/Application Support/Ricochet/Ricochet-Refresh
                configPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Ricochet/Ricochet-Refresh");
            } else {
                configPath = appBundlePath() + QStringLiteral("/config.ricochet");
            }
#else
            configPath = qApp->applicationDirPath() + QStringLiteral("/config");
#endif
            return configPath;
        }();
        configPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

        logger::println("configPath : {}", configPath);
        logger::println("legacyConfigPath : {}", legacyConfigPath);
        bool migrate = false;

        // only put up migration UX when
        if (// old path differs from new path
            configPath != legacyConfigPath &&
            // the old path exists
            QFile::exists(legacyConfigPath) &&
            // the new path does not exist
            !QFile::exists(configPath)) {

            std::cout << "Ricochet Refresh has detected an existing legacy profile." << std::endl;
            std::cout << "Old profile: " << legacyConfigPath.toStdString() << std::endl;
            std::cout << "New profile: " << configPath.toStdString() << std::endl;

            if (migrate) {
                if(!QDir().rename(legacyConfigPath, configPath)) {
                    errorMessage = QStringLiteral("Unable to migrate profile");
                    return false;
                }
                std::cout << "migrated to the new profile path: " << configPath.toStdString() << std::endl;
            } else {
                // use old profile path
                configPath = legacyConfigPath;
                std::cout << "using the old profile path: " << configPath.toStdString() << std::endl;
            }
        }
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

    if(parser.isSet(opt_irc_password) || settings->root()->read("irc.password", QStringLiteral("")) == QStringLiteral("")) {
        settings->root()->write("irc.password", randomPassword());
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

QString randomPassword(size_t len) {
   QString result;
   const QString alphabet("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   auto rng = QRandomGenerator::securelySeeded();
   for(size_t i = 0; i < len; i++) {
       result.append(alphabet.at(rng.bounded(alphabet.length())));
   }
   return result;
}
