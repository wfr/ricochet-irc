#include "main-shared.h"
#include "core/IdentityManager.h"
#include "tor/TorManager.h"
#include "tor/TorControl.h"
#include "tor/TorProcess.h"
#include "utils/CryptoKey.h"
#include "utils/SecureRNG.h"
#include "utils/Settings.h"
#include "irc/RicochetIrcServer.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QTime>
#include <QDir>
#include <QTranslator>
#include <QLocale>
#include <QLockFile>
#include <QStandardPaths>
#include <QMetaObject>
#include <openssl/crypto.h>

#include <QDebug>
#include <QtCore>


class Task : public QObject
{
    Q_OBJECT
public:
    Task(QCoreApplication *app) : QObject(app) { this->args = app->arguments(); }

private:
    RicochetIrcServer *ircServer;
    QStringList args;

public slots:
    void run()
    {
        Tor::TorManager *torManager = Tor::TorManager::instance();
        Tor::TorProcess *proc = torManager->process();
        QObject::connect(proc,
                         SIGNAL(stop()),
                         this,
                         SLOT(torStopped()));
        QObject::connect(torManager,
                         SIGNAL(configurationNeededChanged()),
                         this,
                         SLOT(torConfigurationNeededChanged()));
        QObject::connect(torManager->control(),
                         SIGNAL(torStatusChanged(int, int)),
                         this,
                         SLOT(torStatusChanged(int, int)));


        int port = 6667;
        if(args.length() == 3 && args[1] == QStringLiteral("--port"))
        {
            port = args[2].toInt();
        }

        ircServer = new RicochetIrcServer(this, port);
        QMetaObject::invokeMethod(ircServer, "run");

        //emit finished();
    }

    void torConfigurationNeededChanged()
    {
        Tor::TorManager *torManager = Tor::TorManager::instance();
        qDebug() << "==== Tor configuration needed ====";
        QVariantMap conf;
        /* TODO: make Tor configurable */
        // conf.insert(QStringLiteral("Socks4Proxy"), 0);
        // conf.insert(QStringLiteral("Socks5Proxy"), 0);
        // conf.insert(QStringLiteral("Socks5ProxyUsername"), 0);
        // conf.insert(QStringLiteral("Socks5ProxyPassword"), 0);
        // conf.insert(QStringLiteral("HTTPProxy"), 0);
        // conf.insert(QStringLiteral("HTTPProxyAuthenticator"), 0);
        // conf.insert(QStringLiteral("FirewallPorts"), 0);
        conf.insert(QStringLiteral("FascistFirewall"), 0);
        // conf.insert(QStringLiteral("Bridge"), 0);
        conf.insert(QStringLiteral("UseBridges"), 0);
        conf.insert(QStringLiteral("DisableNetwork"), 0);
        torManager->control()->setConfiguration(conf);
    }

    void torStatusChanged(int newStatus, int oldStatus)
    {
        if(newStatus == Tor::TorControl::TorStatus::TorReady)
        {
            qDebug() << "=== TOR READY ===";
            UserIdentity *identity;
            foreach(identity, identityManager->identities())
            {
                qDebug() << "Identity: " << identity->hostname();
// old test code, delete this, just for reference...
//                identity->contacts.createContactRequest(
//                            "ricochet:mjiesqmikh67vqbo",
//                            "io",
//                            "",
//                            "Hello world");
//                ContactUser *contact;
//                foreach(contact, identity->getContacts()->contacts())
//                {
//                    ConversationModel* conv = contact->conversation();
//                    QMetaObject::invokeMethod((QObject*)conv, "sendMessage", Qt::QueuedConnection, Q_ARG(QString, "Hello!"));
//                }

            }
        }
    }

    void torStopped()
    {
        qDebug() << "tor stopped";
        emit finished();
    }

signals:
    void finished();
};

#include "irc-main.moc"

int main(int argc, char *argv[])
{
    /* Disable rwx memory.
       This will also ensure full PAX/Grsecurity protections. */
    qputenv("QV4_FORCE_INTERPRETER",  "1");
    qputenv("QT_ENABLE_REGEXP_JIT",   "0");

    QCoreApplication app(argc, argv);
    app.setApplicationVersion(QLatin1String("1.1.2"));
    app.setOrganizationName(QStringLiteral("Ricochet"));
    qSetMessagePattern(QString::fromLatin1("%{file}(%{line}): %{message}"));

    QScopedPointer<SettingsFile> settings(new SettingsFile);
    SettingsObject::setDefaultFile(settings.data());

    QString error;
    QLockFile *lock = 0;
    if (!Application::initSettings(settings.data(), &lock, error)) {
        qCritical() << error;
        return 1;
    }
    QScopedPointer<QLockFile> lockFile(lock);

    Application::initTranslation();

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
    torManager->start();

    /* Identities */
    identityManager = new IdentityManager;
    QScopedPointer<IdentityManager> scopedIdentityManager(identityManager);

    Task *task = new Task(&app);

    QObject::connect(task, SIGNAL(finished()), &app, SLOT(quit()));
    QMetaObject::invokeMethod( task, "run", Qt::QueuedConnection );

    return app.exec();
}
