/* Ricochet-IRC - https://github.com/wfr/ricochet-irc/
 * Wolfgang Frisch <wfr@roembden.net>
 *
 * Derived from:
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
#include "core/ContactIDValidator.h"
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
    this->virtual_clients.append(ricochet_user);
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


/**
 * @brief RicochetIrcServer::initRicochet  Prepare the magical intertubes.
 */
void RicochetIrcServer::initRicochet()
{
    if(identityManager->identities().length() != 1)
    {
        qWarning() << "not one identity! don't know what to do...";
        return;
    }
    identity = identityManager->identities()[0];

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
        QObject::connect(convo,
                         SIGNAL(unreadCountChanged()),
                         this,
                         SLOT(onUnreadCountChanged()));
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


/**
 * @brief RicochetIrcServer::startRicochet  (Re)start Tor
 */
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


/**
 * @brief RicochetIrcServer::stopRicochet  Stop Tor (when the IRC user disconnects).
 */
void RicochetIrcServer::stopRicochet()
{
    Tor::TorManager *torManager = Tor::TorManager::instance();
    Tor::TorProcess *proc = torManager->process();
    if(proc && proc->state() != Tor::TorProcess::State::NotStarted)
    {
        torManager->stop();
    }
}


/**
 * @brief RicochetIrcServer::torConfigurationNeededChanged  Hardcoded Tor configuration
 *
 * TODO: make this configurable
 */
void RicochetIrcServer::torConfigurationNeededChanged()
{
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
    connect(torManager->control()->setConfiguration(conf),
            SIGNAL(finished()), SLOT(torConfigurationFinished()));
}


void RicochetIrcServer::torConfigurationFinished()
{
    Tor::TorManager *torManager = Tor::TorManager::instance();
    torManager->control()->saveConfiguration();
}



void RicochetIrcServer::torStatusChanged(int newStatus, int oldStatus)
{
    Q_UNUSED(oldStatus);

    switch(newStatus)
    {
        case Tor::TorControl::TorStatus::TorOffline:
            // Workaround for new profiles
            if(Tor::TorManager::instance()->configurationNeeded())
            {
                torConfigurationNeededChanged();
            }
        break;

        case Tor::TorControl::TorStatus::TorReady:
            qDebug() << "=== Tor ready ===";
            UserIdentity *identity;
            foreach(identity, identityManager->identities())
            {
                getChannel(control_channel_name)->setTopic(ricochet_user,identity->contactID());
            }
        break;

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
    getChannel(control_channel_name)->setMemberFlags(conn, QStringLiteral("+o"));
    cmdHelp();

    // start Tor
    if(this->clients.count() == 1)
    {
        startRicochet();
    }
    else if(this->clients.count() > 1)
    {
        qWarning() << "more than one IRC client connected; killing old connections";
        foreach(IrcConnection* client_conn, clients.values())
        {
            if(conn != client_conn)
            {
                quit(client_conn);
                disconnect(client_conn);
            }
        }
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


/**
 * @brief RicochetIrcServer::echo  Print in the #ricochet channel.
 * @param text
 */
void RicochetIrcServer::echo(const QString &text)
{
    privmsg(ricochet_user, control_channel_name, text);
}


/**
 * @brief RicochetIrcServer::error  Print an error in #ricochet.
 * @param text
 */
void RicochetIrcServer::error(const QString &text)
{
    echo(QStringLiteral("ERROR: ") + text);
}


/**
 * @brief RicochetIrcServer::highlight Draw attention to the user ...
 * ... by highlighting them in #ricochet
 */
void RicochetIrcServer::highlight()
{
    IrcChannel *cc = getChannel(control_channel_name);
    foreach(IrcUser* user, clients.values()) {
        if(cc->hasMember(user->nick)) {
            echo(QStringLiteral("%1: Attention!").arg(user->nick));
        }
    }
}


/**
 * @brief RicochetIrcServer::privmsgHook  Handle IRC messages from the user
 * @param sender
 * @param msgtarget  can be a #channel or a nickname
 * @param text
 */
void RicochetIrcServer::privmsgHook(IrcUser* sender, const QString& msgtarget, const QString& text)
{
    IrcConnection* sender_conn = qobject_cast<IrcConnection*>(sender);

    if(msgtarget == control_channel_name && sender->nick != ricochet_user->nick)
    {
        QStringList args = text.trimmed().split(QLatin1Char(' '));
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
        handlePM(sender_conn, msgtarget, text);
    }
}


/**
 * @brief RicochetIrcServer::handlePM  Forward outgoing IRC messages to Ricochet.
 * @param sender IRC user who sent the message
 * @param contact_nick Nickname of the Ricochet contact
 * @param text
 *
 * Notify the IRC user if the contact is offline.
 */
void RicochetIrcServer::handlePM(IrcConnection *sender, const QString &contact_nick, const QString &text)
{
    foreach(ContactUser* contact, usermap.keys())
    {
        if(contact->nickname() == contact_nick)
        {
            contact->conversation()->sendMessage(text);

            IrcUser* contact_irc_user = usermap.value(contact);
            if(!contact->isConnected())
            {
                sender->reply(contact_irc_user->getPrefix(),
                            QStringLiteral("NOTICE %1 :\x02[Contact is offline. %2 queued message(s).]")
                            .arg(sender->getPrefix())
                            .arg(contact->conversation()->queuedCount()));

            }
        }
    }
}


void RicochetIrcServer::cmdHelp()
{
    // figlet -f small <<< "Ricochet IRC v3"
    echo(QStringLiteral(" ___ _            _        _     ___ ___  ___       ____"));
    echo(QStringLiteral("| _ (_)__ ___  __| |_  ___| |_  |_ _| _ \\/ __| __ _|__ /"));
    echo(QStringLiteral("|   / / _/ _ \\/ _| ' \\/ -_)  _|  | ||   / (__  \\ V /|_ \\"));
    echo(QStringLiteral("|_|_\\_\\__\\___/\\__|_||_\\___|\\__| |___|_|_\\\\___|  \\_/|___/ %1").arg(QCoreApplication::applicationVersion()));
    echo(QStringLiteral(""));
    echo(QStringLiteral("COMMANDS:"));
    echo(QStringLiteral(" * help"));
    echo(QStringLiteral(" * id                            -- print your ricochet id"));
    echo(QStringLiteral(" * add ID NICKNAME MESSAGE       -- add a contact"));
    echo(QStringLiteral(" * delete NICKNAME               -- delete a contact"));
    echo(QStringLiteral(" * rename NICKNAME NEW_NICKNAME  -- rename a contact"));
    echo(QStringLiteral(" * request list                  -- list incoming requests"));
    echo(QStringLiteral(" * request accept ID NICKNAME    -- accept incoming request"));
    echo(QStringLiteral(" * request reject ID             -- reject incoming request"));
    echo(QStringLiteral(""));
}


void RicochetIrcServer::cmdId()
{
    echo(identity->contactID());
}


void RicochetIrcServer::cmdAdd(const QStringList& args)
{
    if(args.length() < 3)
    {
        error(QStringLiteral("Unexpected arguments."));
    }
    else
    {
        QString id = args[0];
        QString nickname = args[1];
        QString message = args[2];
        for(int i=3; i<args.length(); i++)
        {
            message.append(QStringLiteral(" %1").arg(args[i]));
        }
        if(ContactIDValidator::isValidID(id))
        {
            echo(QStringLiteral("sending contact request to user `%1` with id: %2 with message: %3").arg(nickname).arg(id).arg(message));
            ContactsManager *contactsManager = identity->getContacts();
            contactsManager->createContactRequest(id, nickname, identity->nickname(), message);
        }
        else
        {
            error(QStringLiteral("invalid Ricochet ID: %1").arg(id));
        }
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
            virtual_clients.removeOne(usermap[contact]);
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

        if(contactsManager->lookupNickname(new_nickname)
                || findUser(new_nickname) != Q_NULLPTR)
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


/**
 * @brief RicochetIrcServer::onContactStatusChanged  A contact has come online or gone offline.
 * @param user
 * @param status
 *
 * => set +v/-v in #ricochet
 * => tell the IRC user about any unsent essages
 */
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
        virtual_clients.append(ircuser);
    }

    switch(status)
    {
        case ContactUser::Online:
            ctrlchan->setMemberFlags(ircuser, QStringLiteral("+v"));

            foreach(IrcConnection *conn, clients.values())
            {
               conn->reply(RPL_WHOREPLY,
                            QStringLiteral("#ricochet %1 %2 %3 127.0.0.1 %4 H 0 %5")
                            .arg(conn->nick)
                            .arg(ircuser->user)
                            .arg(ircuser->hostname)
                            .arg(ircuser->nick)
                            .arg(ircuser->nick)
                           );
           }

            // Notify user in query when there are queued messages.
            if(user->conversation()->queuedCount() > 0)
            {
                foreach(IrcConnection *conn, clients.values())
                {
                    conn->reply(ircuser->getPrefix(),
                                QStringLiteral("NOTICE %1 :\x02[Contact is back online. Sending queued messages...]")
                                .arg(conn->getPrefix()));
                }
            }
        break;
        case ContactUser::Offline:
            ctrlchan->setMemberFlags(ircuser, QStringLiteral("-v"));

           foreach(IrcConnection *conn, clients.values())
            {
               conn->reply(RPL_WHOREPLY,
                            QStringLiteral("#ricochet %1 %2 %3 127.0.0.1 %4 G 0 %5")
                            .arg(conn->nick)
                            .arg(ircuser->user)
                            .arg(ircuser->hostname)
                            .arg(ircuser->nick)
                            .arg(ircuser->nick)
                           );
           }
        break;
    }
}

/**
 * @brief RicochetIrcServer::onUnreadCountChanged  Forward Ricochet messages to IRC.
 */
void RicochetIrcServer::onUnreadCountChanged()
{
    ConversationModel* convo = qobject_cast<ConversationModel*>(sender());

    while(convo->rowCount())
    {
        QModelIndex index = convo->index(0);
        QString text = convo->data(index, Qt::DisplayRole).toString();
        bool isOutgoing = convo->data(index, ConversationModel::IsOutgoingRole).toBool();
        convo->clear();

        if(!isOutgoing)
        {
            // IRC has no concept of multi-line messages. Split them.
            QStringList lines = text.split(QLatin1Char('\n'));
            foreach(QString line, lines)
            {
                IrcUser *ircuser = usermap[convo->contact()];
                foreach(IrcConnection *conn, clients.values())
                {
                    conn->reply(ircuser->getPrefix(),
                                QStringLiteral("PRIVMSG %1 :%2").arg(conn->nick).arg(line));
                }
            }
        }
    }
}


/**
 * @brief RicochetIrcServer::requestAdded  There is an incoming contact request.
 * @param request Incoming contact request
 */
void RicochetIrcServer::requestAdded(IncomingContactRequest *request)
{
    highlight();
    QString notice = QStringLiteral("Incoming contact request from: %1").arg(request->contactId());
    if(request->message().length() > 0)
    {
        notice += QStringLiteral(" (message: %1)").arg(request->message());
    }
    echo(notice);
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
