#include <QCoreApplication>
#include <QCommandLineParser>
#include <QObject>
#include <QRandomGenerator>

#include "libtego_callbacks.hpp"

#include "shims/TorControl.h"
#include "shims/TorManager.h"
#include "shims/UserIdentity.h"

#include "RicochetIrcServer.h"
#include "RicochetIrcServerTask.h"

bool verbose_output = true;
static QString randomPassword(size_t len = 32);
static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
static bool initSettings(SettingsFile *settings, QLockFile **lockFile, QString &errorMessage);


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qInstallMessageHandler(myMessageOutput);

    tego_context_t* tegoContext = nullptr;
    tego_initialize(&tegoContext, tego::throw_on_error());

    auto tego_cleanup = tego::make_scope_exit([=]() -> void {
        tego_uninitialize(tegoContext, tego::throw_on_error());
    });

    init_libtego_callbacks(tegoContext);


    a.setApplicationVersion(QLatin1String(TEGO_VERSION_STR));
    qDebug() << "tego version:" << TEGO_VERSION_STR;

    QScopedPointer<SettingsFile> settings(new SettingsFile);
    SettingsObject::setDefaultFile(settings.data());

    QString error;
    QLockFile *lock = 0;
    if (!initSettings(settings.data(), &lock, error)) {
        if (error.isEmpty()) {
            return 0;
        }
        qCritical() << "Ricochet Error:" << error;
        return 1;
    }
    QScopedPointer<QLockFile> lockFile(lock);


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
            static_cast<size_t>(rawFilePath.size()),
            tego::throw_on_error());

        tego_context_start_tor(tegoContext, launchConfig.get(), tego::throw_on_error());
    }


    /* Identities */

    // init our shims
    shims::UserIdentity::userIdentity = new shims::UserIdentity(tegoContext);

    // wait until a control connection has been established before attempting
    // to send configuration info to the daemon
    QObject::connect(
        shims::TorControl::torControl,
        &shims::TorControl::statusChanged,
        [&](int newStatus, int) -> void {
            if (newStatus == tego_tor_control_status_connected) {

                // send configuration down to tor daemon
                auto networkSettings = SettingsObject().read("tor").toObject();
                shims::TorControl::torControl->setConfiguration(networkSettings);

                // at this point we could configure Tor,
                // however we're happy with a default Tor config for now.
                shims::TorControl::torControl->beginBootstrap();

                // start up our onion service
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
                    auto contactsManager = shims::UserIdentity::userIdentity->getContacts();

                    // construct privatekey from privateKey keyblob
                    std::unique_ptr<tego_ed25519_private_key_t> privateKey;
                    auto keyBlob = privateKeyString.toUtf8();

                    tego_ed25519_private_key_from_ed25519_keyblob(
                        tego::out(privateKey),
                        keyBlob.data(),
                        static_cast<size_t>(keyBlob.size()),
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
                            static_cast<size_t>(serviceIdRaw.size()),
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
            }
        });

    // Start the IRC server
    auto task = new RicochetIrcServerTask(&a);
    QObject::connect(task, &RicochetIrcServerTask::finished, &a, QCoreApplication::quit);

    QMetaObject::invokeMethod( task, "run", Qt::QueuedConnection );

    return a.exec();
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


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        if (::verbose_output) {
            fprintf(stderr, "%s\n", localMsg.constData());
        }
        break;
    case QtInfoMsg:
    case QtWarningMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "CRITICAL: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "FATAL: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}


// Writes default settings to settings object. Does not care about any
// preexisting values, therefore this is best used on a fresh object.
static void loadDefaultSettings(SettingsFile *settings)
{
    settings->root()->write("ui.combinedChatWindow", true);
}


static bool initSettings(SettingsFile *settings, QLockFile **lockFile, QString &errorMessage)
{
    /* ricochet-refresh by default loads and saves configuration files from QStandardPaths::AppConfigLocation
     *
     * Linux: ~/.config/ricochet-refresh
     * Windows: C:/Users/<USER>/AppData/Local/ricochet-refresh
     * macOS: ~/Library/Preferences/<APPNAME>
     *
     * ricochet-refresh can also load configuration files from a custom directory passed in as the first argument
     */
    QString defaultConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    // Parse command-line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main","Anonymous peer-to-peer instant messaging, IRC gateway"));
    QCommandLineOption opt_config_path(QStringLiteral("config"),
                                       QCoreApplication::translate("main", "Select configuration directory."),
                                       QStringLiteral("config-path"),
                                       defaultConfigPath);
    parser.addOption(opt_config_path);
    QCommandLineOption opt_irc_port(QStringLiteral("port"),
                                    QCoreApplication::translate("irc", "Set IRC server port."),
                                    QStringLiteral("port"),
                                    QStringLiteral("6667"));
    parser.addOption(opt_irc_port);
    QCommandLineOption opt_irc_password(QStringLiteral("generate-password"),
                                        QCoreApplication::translate("irc", "Generate random IRC password."));
    parser.addOption(opt_irc_password);
    QCommandLineOption opt_verbose(QStringList() << "debug" << "verbose",
                                   QCoreApplication::translate("irc", "Verbose output"));
    parser.addOption(opt_verbose);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(qApp->arguments());

    if(parser.positionalArguments().count() > 0) {
        parser.showHelp(1);
        return false;
    }

    if (parser.isSet(opt_verbose)) {
        ::verbose_output = true;
    }

    // Load/initialize config
    QDir dir(parser.value(opt_config_path));
    logger::println("configPath : {}", dir);

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

    // IRC settings
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

    if(parser.isSet(opt_irc_password)
        || settings->root()->read("irc.password", QStringLiteral("")) == QStringLiteral("")) {
        settings->root()->write("irc.password", randomPassword());
    }

    return true;
}

