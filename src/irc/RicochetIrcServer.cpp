#include "RicochetIrcServer.h"
#include "IrcServer.h"
#include "IrcUser.h"
#include "IrcChannel.h"
#include "IrcConnection.h"

#include "core/IdentityManager.h"
#include "core/ConversationModel.h"
#include "core/ContactsManager.h"
#include "core/ContactUser.h"
#include "core/UserIdentity.h"
#include "core/IncomingRequestManager.h"
#include "tor/TorManager.h"
#include "tor/TorControl.h"
#include "tor/TorProcess.h"

#include <QCoreApplication>


RicochetIrcServer::RicochetIrcServer(QObject *parent, uint16_t port, const QString& password, const QString& control_channel_name)
    : IrcServer(parent, port, password),
      control_channel_name(control_channel_name),
      first_start(true)
{
}


RicochetIrcServer::~RicochetIrcServer()
{
    delete ricochet_user;
}

bool RicochetIrcServer::run()
{
    if(!IrcServer::run())
    {
        return false;
    }

    this->welcome_message = QCoreApplication::translate("irc", "Welcome to Ricochet-IRC!");

    // Create the control user @ricochet
    ricochet_user = new IrcUser(this);
    ricochet_user->nick = QStringLiteral("ricochet");
    ricochet_user->user = QStringLiteral("ricochet");
    ricochet_user->hostname = QStringLiteral("::1");
    ricochet_user->realname = QStringLiteral("Ricochet Control User");
    QObject::connect(ricochet_user,
                     SIGNAL(privmsg(IrcUser*, const QString&, const QString&)),
                     this,
                     SLOT(privmsg(IrcUser*, const QString&, const QString&)));

    // Create the control channel
    IrcChannel *ricochet_channel = getChannel(control_channel_name);
    ricochet_channel->addMember(ricochet_user, QStringLiteral("@"));
    channels.insert(ricochet_channel->name, ricochet_channel);

    return true;
}

void RicochetIrcServer::initRicochet()
{
    identity = identityManager->identities()[0];
    if(identityManager->identities().length() != 1)
    {
        qWarning() << "not one identity! don't know what to do...";
    }

    // Connect Ricochet contact events to IRC
    ContactsManager *contactsManager = identity->getContacts();
    QObject::connect(contactsManager,
                     SIGNAL(contactAdded(ContactUser*)),
                     this,
                     SLOT(onContactAdded(ContactUser*)));
    QObject::connect(contactsManager,
                     SIGNAL(contactStatusChanged(ContactUser*,int)),
                     this,
                     SLOT(onContactStatusChanged(ContactUser*,int)));
    foreach(ContactUser* user, contactsManager->contacts())
    {
        ConversationModel* convo = user->conversation();
        QObject::connect(convo, SIGNAL(unreadCountChanged()), this, SLOT(onUnreadCountChanged()));
    }

    QObject::connect(contactsManager->incomingRequestManager(),
                     SIGNAL(requestAdded(IncomingContactRequest*)),
                     this,
                     SLOT(requestAdded(IncomingContactRequest*)));
    QObject::connect(contactsManager->incomingRequestManager(),
                     SIGNAL(requestRemoved(IncomingContactRequest*)),
                     this,
                     SLOT(requestRemoved(IncomingContactRequest*)));
    QObject::connect(contactsManager->incomingRequestManager(),
                     SIGNAL(requestsChanged()),
                     this,
                     SLOT(requestsChanged()));
}

void RicochetIrcServer::startRicochet()
{
    if(first_start) {
        initRicochet();
    }

    Tor::TorManager *torManager = Tor::TorManager::instance();
    Tor::TorProcess *proc = torManager->process();
    torControl = torManager->control();

    if(!first_start)
    {
        torControl->reconnect();
    }

    if(proc == Q_NULLPTR || proc->state() == Tor::TorProcess::State::NotStarted)
    {
        torManager->start();
        if(first_start)
        {
            QObject::connect(torManager,
                             SIGNAL(configurationNeededChanged()),
                             this,
                             SLOT(torConfigurationNeededChanged()));
            QObject::connect(torManager->control(),
                             SIGNAL(torStatusChanged(int, int)),
                             this,
                             SLOT(torStatusChanged(int, int)));
        }
    }

    first_start = false;
}

void RicochetIrcServer::stopRicochet()
{
    Tor::TorManager *torManager = Tor::TorManager::instance();
    Tor::TorProcess *proc = torManager->process();
    if(proc->state() != Tor::TorProcess::State::NotStarted)
    {
        torManager->stop();
    }
}


void RicochetIrcServer::torConfigurationNeededChanged()
{
    // TODO: make Tor configurable via IRC
    qDebug() << "==== Tor configuration needed ====";
    Tor::TorManager *torManager = Tor::TorManager::instance();
    QVariantMap conf;
    conf.insert(QStringLiteral("FascistFirewall"), 0);
    conf.insert(QStringLiteral("UseBridges"), 0);
    conf.insert(QStringLiteral("DisableNetwork"), 0);
    // conf.insert(QStringLiteral("Socks4Proxy"), 0);
    // conf.insert(QStringLiteral("Socks5Proxy"), 0);
    // conf.insert(QStringLiteral("Socks5ProxyUsername"), 0);
    // conf.insert(QStringLiteral("Socks5ProxyPassword"), 0);
    // conf.insert(QStringLiteral("HTTPProxy"), 0);
    // conf.insert(QStringLiteral("HTTPProxyAuthenticator"), 0);
    // conf.insert(QStringLiteral("FirewallPorts"), 0);
    // conf.insert(QStringLiteral("Bridge"), 0);
    torManager->control()->setConfiguration(conf);
    torManager->control()->saveConfiguration();
}

void RicochetIrcServer::torStatusChanged(int newStatus, int oldStatus)
{
    Q_UNUSED(oldStatus);

    if(newStatus == Tor::TorControl::TorStatus::TorReady)
    {
        qDebug() << "=== Tor ready ===";
        UserIdentity *identity;
        foreach(identity, identityManager->identities())
        {
            getChannel(control_channel_name)->setTopic(ricochet_user,identity->contactID());
        }
    }

    if(clients.count() > 0)
    {
        switch(newStatus) {
            case Tor::TorControl::TorStatus::TorUnknown:
                echo(QStringLiteral("Tor status: unknown"));
                break;
            case Tor::TorControl::TorStatus::TorOffline:
                echo(QStringLiteral("Tor status: offline"));
                break;
            case Tor::TorControl::TorStatus::TorReady:
                echo(QStringLiteral("Tor status: ready"));

                // add all contacts to IRC
                ContactsManager *contactsManager = identity->getContacts();
                foreach(ContactUser* contact, contactsManager->contacts()) {
                    emit onContactStatusChanged(contact, ContactUser::Offline);
                }

                break;
        }
    }
}

const QString RicochetIrcServer::getWelcomeMessage()
{
    return welcome_message;
}

void RicochetIrcServer::ircUserLoggedIn(IrcConnection* conn)
{
    qDebug() << "ircUserLoggedIn -- # of connections: " << this->clients.count();
    conn->join(control_channel_name);
    getChannel(control_channel_name)->setMemberFlags(conn, QStringLiteral("+@"));
    echo(QStringLiteral(" ___ _            _        _     ___ ___  ___ "));
    echo(QStringLiteral("| _ (_)__ ___  __| |_  ___| |_  |_ _| _ \\/ __|"));
    echo(QStringLiteral("|   / / _/ _ \\/ _| ' \\/ -_)  _|  | ||   / (__ "));
    echo(QStringLiteral("|_|_\\_\\__\\___/\\__|_||_\\___|\\__| |___|_|_\\\\___|"));
    //echo(QCoreApplication::translate("irc", "Please turn off logging in your IRC client."));
    echo(QStringLiteral(""));
    cmdHelp();
    echo(QStringLiteral(""));

    // start Tor
    if(this->clients.count() == 1)
    {
        startRicochet();
    }
    else if(this->clients.count() > 1)
    {
        qWarning() << "more than one IRC client connected; undefined behavior";
    }
}

void RicochetIrcServer::ircUserLeft(IrcConnection* conn)
{
    Q_UNUSED(conn);
    qDebug() << "ircUserLeft     -- # of connections: " << this->clients.count();

    if(this->clients.count() == 0)
    {
        stopRicochet();
    }
}

void RicochetIrcServer::echo(const QString &text)
{
    privmsg(ricochet_user, control_channel_name, text);
}

void RicochetIrcServer::error(const QString &text)
{
    echo(QStringLiteral("ERROR: ") + text);
}


void RicochetIrcServer::privmsgHook(IrcUser* sender, const QString& msgtarget, const QString& text)
{
    if(msgtarget == control_channel_name && sender->nick != ricochet_user->nick)
    {
        QStringList args = text.split(QLatin1Char(' '));
        QString cmd = args[0];
        args.pop_front();

        if(cmd == QStringLiteral("help"))
        {
            cmdHelp();
        }
        else
        if(cmd == QStringLiteral("id"))
        {
            cmdId();
        }
        else
        if(cmd == QStringLiteral("add"))
        {
            cmdAdd(args);
        }
        else
        if(cmd == QStringLiteral("delete"))
        {
            cmdDelete(args);
        }
        else
        if(cmd == QStringLiteral("rename"))
        {
            cmdRename(args);
        }
        else
        if(cmd == QStringLiteral("request"))
        {
            cmdRequest(args);
        }
        else
        {
            error(QStringLiteral("Unknown command: %1").arg(cmd));
        }
    }
    else
    {
        foreach(ContactUser* contactuser, usermap.keys())
        {
            if(contactuser->nickname() == msgtarget)
            {
                if(!contactuser->isConnected())
                {
                    IrcConnection* conn = qobject_cast<IrcConnection*>(sender);
                    conn->reply(RPL_AWAY,
                                QStringLiteral("%1 %2 :is offline")
                                .arg(sender->nick)
                                .arg(contactuser->nickname())
                                );
                }
                contactuser->conversation()->sendMessage(text);
            }
        }
    }
}


void RicochetIrcServer::cmdHelp()
{
    echo(QStringLiteral("COMMANDS:"));
    echo(QStringLiteral(" * help"));
    echo(QStringLiteral(" * id                      -- print your ricochet id"));
    echo(QStringLiteral(" * add ID NICK MESSAGE     -- add a contact"));
    echo(QStringLiteral(" * delete NICK             -- delete a contact"));
    echo(QStringLiteral(" * rename NICK NEW_NICK    -- rename a contact"));
    echo(QStringLiteral(" * request list            -- list incoming requests"));
    echo(QStringLiteral(" * request accept ID NICK  -- accept incoming request"));
    echo(QStringLiteral(" * request reject ID       -- reject incoming request"));
}


void RicochetIrcServer::cmdId()
{
    echo(identity->contactID());
}


void RicochetIrcServer::cmdAdd(const QStringList& args)
{
    if(args.length() != 3)
    {
        error(QStringLiteral("Unexpected arguments."));
    }
    else
    {
        // TODO: validate arguments
        QString id = args[0];
        QString nickname = args[1];
        QString message = args[2];
        echo(QStringLiteral("sending contact request to user `%1` with id: %2").arg(nickname).arg(id));;

        ContactsManager *contactsManager = identity->getContacts();
        contactsManager->createContactRequest(id, nickname, identity->nickname(), message);
    }
}


void RicochetIrcServer::cmdDelete(const QStringList& args)
{
    if(args.length() == 1)
    {
        ContactsManager *contactsManager = identity->getContacts();
        ContactUser *contact = contactsManager->lookupNickname(args[0]);
        if(contact)
        {
            contact->deleteContact();
            quit(usermap.value(contact));
            usermap.remove(contact);
        }
        else
        {
            echo(QStringLiteral("Contact not found."));
        }
    }
    else
    {
        error(QStringLiteral("Unexpected arguments."));
    }
}


void RicochetIrcServer::cmdRename(const QStringList& args)
{
    if(args.length() == 2)
    {
        QString old_nickname = args[0];
        QString new_nickname = args[1];

        ContactsManager *contactsManager = identity->getContacts();

        if(contactsManager->lookupNickname(new_nickname))
        {
            error(QStringLiteral("Target nickname `%1` already exists.").arg(new_nickname));
        }
        else
        {
            ContactUser *contact = contactsManager->lookupNickname(old_nickname);
            if(contact)
            {
                echo(QStringLiteral("renaming user `%1` to `%2`").arg(old_nickname).arg(new_nickname));
                contact->setNickname(new_nickname);
                IrcUser* ircuser = usermap.value(contact);
                if(ircuser != Q_NULLPTR)
                {
                    this->rename(ircuser, new_nickname);
                }
            }
            else
            {
                echo(QStringLiteral("Contact not found."));
            }
        }
    }
    else
    {
        error(QStringLiteral("Unexpected arguments."));
    }
}


IncomingContactRequest* RicochetIrcServer::getIncomingRequestByID(const QString& id)
{
    IncomingContactRequest *result = Q_NULLPTR;
    ContactsManager *contactsManager = identity->getContacts();
    foreach(IncomingContactRequest *request, contactsManager->incomingRequestManager()->requests())
    {
        if(request->contactId() == id)
        {
            result = request;
        }
    }
    return result;
}


void RicochetIrcServer::cmdRequest(const QStringList& args)
{
    // TODO: validate arguments
    ContactsManager *contactsManager = identity->getContacts();
    IncomingRequestManager *irm = contactsManager->incomingRequestManager();

    if(args.length() == 1 && args[0] == QStringLiteral("list"))
    {
        echo(QStringLiteral("You have %1 incoming contact request(s):").arg(irm->requests().count()));
        foreach(IncomingContactRequest *inc, irm->requests())
        {
            echo(QStringLiteral("    %1 (message: %2)")
                 .arg(inc->contactId()).arg(inc->message()));
        }
    }
    else
    if(args.length() == 3 && args[0] == QStringLiteral("accept"))
    {
        IncomingContactRequest *request = getIncomingRequestByID(args[1]);
        if(request)
        {
            request->setNickname(args[2]);
            request->accept();
        }
        else
        {
            echo(QStringLiteral("No such request was found."));
        }
    }
    else
    if(args.length() == 2 && args[0] == QStringLiteral("reject"))
    {
        IncomingContactRequest *request = getIncomingRequestByID(args[1]);
        if(request)
        {
            request->reject();
        }
        else
        {
            echo(QStringLiteral("No such request was found."));
        }
    }
    else
    {
        error(QStringLiteral("Unexpected arguments."));
    }
}


void RicochetIrcServer::onContactAdded(ContactUser *user)
{
    QObject::connect(user->conversation(),
                     SIGNAL(unreadCountChanged()),
                     this,
                     SLOT(onUnreadCountChanged()));
}


void RicochetIrcServer::onContactStatusChanged(ContactUser* user, int status)
{
    IrcChannel *ctrlchan = channels[control_channel_name];
    IrcUser *ircuser;

    if(ctrlchan->hasMember(user->nickname()))
    {
        ircuser = ctrlchan->getMember(user->nickname());
    }
    else
    {
        ircuser = new IrcUser(this);
        ircuser->nick = user->nickname();
        ircuser->user = user->nickname();
        ircuser->hostname = user->contactID();
        ctrlchan->addMember(ircuser, QStringLiteral("+v"));
        joined(ircuser, control_channel_name);
        usermap.insert(user, ircuser);
    }

    switch(status)
    {
        case ContactUser::Online:
            ctrlchan->setMemberFlags(ircuser, QStringLiteral("+v"));
        break;
        case ContactUser::Offline:
            ctrlchan->setMemberFlags(ircuser, QStringLiteral("-v"));
        break;
    }
}

void RicochetIrcServer::onUnreadCountChanged()
{
    ConversationModel* convo = qobject_cast<ConversationModel*>(sender());

    while(convo->rowCount())
    {
        QString text = convo->data(convo->index(0)).toString();
        convo->clear();

        IrcUser *ircuser = usermap[convo->contact()];
        foreach(IrcConnection *conn, clients.values())
        {
            conn->reply(ircuser->getPrefix(), QStringLiteral("PRIVMSG %1 :%2").arg(conn->nick).arg(text));
        }
    }
}


void RicochetIrcServer::requestAdded(IncomingContactRequest *request)
{
    echo(QStringLiteral("Incoming contact request from: %1 (message: %2)")
         .arg(request->contactId()).arg(request->message()));
}


void RicochetIrcServer::requestRemoved(IncomingContactRequest *request)
{
    qDebug() << QStringLiteral("Request removed: %1, message: %2")
         .arg(request->contactId()).arg(request->message());
}


void RicochetIrcServer::requestsChanged()
{
    qDebug() << "Contact requests changed";
}
