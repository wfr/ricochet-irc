#include "RicochetIrcServerTask.h"
#include "RicochetIrcServer.h"

#include "ui/ContactsModel.h"

#include "utils/Settings.h"

// shim replacements
#include "shims/TorControl.h"
#include "shims/TorManager.h"
#include "shims/UserIdentity.h"
#include "shims/ContactsManager.h"
#include "shims/ContactUser.h"
#include "shims/ConversationModel.h"
#include "shims/OutgoingContactRequest.h"
#include "shims/ContactIDValidator.h"
#include "shims/IncomingContactRequest.h"

#include <iostream>

RicochetIrcServerTask::RicochetIrcServerTask(QCoreApplication* app)
    : QObject(app), irc_server(nullptr)
{
    SettingsObject settings;
    uint16_t port = static_cast<uint16_t>(settings.read("irc.port", 6667).toInt());
    QString password = settings.read("irc.password", QLatin1String("")).toString();

    irc_server = new RicochetIrcServer(this, port, password);
}

void RicochetIrcServerTask::run()
{
    bool result;
    QMetaObject::invokeMethod(irc_server, "run", Q_RETURN_ARG(bool, result));
    if(!result)
    {
        qDebug() << "!result";
        emit finished();
    }

    std::cout << std::endl;
    std::cout << "## IRC server started:" << std::endl;
    std::cout << "Host:      127.0.0.1" << std::endl;
    std::cout << "Port:      " << irc_server->port() << std::endl;
    std::cout << "Password:  " << irc_server->password().toUtf8().data() << std::endl;
    std::cout << std::endl;
    std::cout << "### WeeChat client setup:" << std::endl;
    std::cout << "/server add ricochet 127.0.0.1/" << irc_server->port()  << std::endl;
    std::cout << "/set irc.server.ricochet.password \"" << irc_server->password().toUtf8().data() << "\"" << std::endl;
    std::cout << "/set logger.level.irc.ricochet 0" << std::endl;
    std::cout << std::endl;
    std::cout.flush();
}
