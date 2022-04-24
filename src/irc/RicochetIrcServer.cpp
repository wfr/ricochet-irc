/* Ricochet-IRC - https://github.com/wfr/ricochet-irc/
 * Wolfgang Frisch <wfrisch@riseup.net>
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

#include "shims/ContactIDValidator.h"
#include "shims/IncomingContactRequest.h"
#include "shims/TorControl.h"
#include "shims/TorManager.h"

#include <QCoreApplication>


RicochetIrcServer::RicochetIrcServer(QObject *parent,
                                     uint16_t port, const QString& password,
                                     const QString& control_channel_name)
    : IrcServer(parent, port, password),
      control_channel_name(control_channel_name)
{

}


RicochetIrcServer::~RicochetIrcServer()
{
    delete ricochet_user;
}


/**
 * @brief RicochetIrcServer::initRicochet
 */
void RicochetIrcServer::initRicochet()
{
    connect(shims::TorManager::torManager,
            &shims::TorManager::configurationNeededChanged,
            this,
            &RicochetIrcServer::torConfigurationNeededChanged);
    connect(shims::TorManager::torManager,
            &shims::TorManager::runningChanged,
            this,
            &RicochetIrcServer::torRunningChanged);
    connect(shims::TorManager::torManager,
            &shims::TorManager::errorChanged,
            this,
            &RicochetIrcServer::torErrorChanged);

    auto userIdentity = shims::UserIdentity::userIdentity;
    auto contactsManager = shims::UserIdentity::userIdentity->getContacts();

    connect(contactsManager,
            &shims::ContactsManager::contactAdded,
            this,
            &RicochetIrcServer::onContactAdded);
    connect(contactsManager,
            &shims::ContactsManager::contactStatusChanged,
            this,
            &RicochetIrcServer::onContactStatusChanged);
    foreach(shims::ContactUser* user, contactsManager->contacts())
    {
        connect(user->conversation(),
                &shims::ConversationModel::unreadCountChanged,
                this,
                &RicochetIrcServer::onUnreadCountChanged);
    }
    connect(userIdentity,
            &shims::UserIdentity::requestAdded,
            this,
            &RicochetIrcServer::requestAdded);
// TODO: this signal is not available anymore
//    QObject::connect(contactsManager->incomingRequestManager(),
//                     SIGNAL(requestRemoved(IncomingContactRequest*)),
//                     this,
//                     SLOT(requestRemoved(IncomingContactRequest*)));
    connect(userIdentity,
            &shims::UserIdentity::requestsChanged,
            this,
            &RicochetIrcServer::requestsChanged);
}

bool RicochetIrcServer::run()
{
    if(!IrcServer::run())
    {
        return false;
    }

    initRicochet();

    this->welcome_message = QCoreApplication::translate("irc", "Welcome to Ricochet-IRC!");

    // Create the control user @ricochet
    ricochet_user = new IrcUser(this);
    ricochet_user->nick = QStringLiteral("ricochet");
    ricochet_user->user = QStringLiteral("ricochet");
    ricochet_user->hostname = QStringLiteral("::1");
    ricochet_user->realname = QStringLiteral("Ricochet Control User");
    this->virtual_clients.append(ricochet_user);
    connect(ricochet_user,
            &IrcUser::privmsg,
            this,
            &RicochetIrcServer::privmsg);

    // Create the control channel
    IrcChannel *ricochet_channel = getChannel(control_channel_name);
    ricochet_channel->addMember(ricochet_user, QStringLiteral("@"));
    channels.insert(ricochet_channel->name, ricochet_channel);

    return true;
}

/**
 * @brief RicochetIrcServer::startRicochet  (Re)start Tor
 */
void RicochetIrcServer::startRicochet()
{
    QVariantMap conf = {
        { QStringLiteral("disableNetwork"), 0 }
    };
    auto command = qobject_cast<shims::TorControlCommand*>(shims::TorControl::torControl->setConfiguration(conf));
    QObject::connect(command, &shims::TorControlCommand::finished, this, &RicochetIrcServer::torConfigurationFinished);
}


/**
 * @brief RicochetIrcServer::stopRicochet  Stop Tor (when the IRC user disconnects).
 */
void RicochetIrcServer::stopRicochet()
{
    // Problem: Tor won't restart after this:
    // tego_context_stop_tor()
    QVariantMap conf = {
        { QStringLiteral("disableNetwork"), 1 }
    };
    shims::TorControl::torControl->setConfiguration(conf);
}


/**
 * @brief RicochetIrcServer::torConfigurationNeededChanged  Hardcoded Tor configuration
 *
 * BUG: This slot is currently never called because the signal in TorManager
 *      is emitted in its constructor, before this class is instantiated.
 */
void RicochetIrcServer::torConfigurationNeededChanged()
{
//    auto& torControl = shims::TorControl::torControl;
//    qDebug() << "==== IRC torConfigurationNeededChanged() ====";
//    QVariantMap conf = {
//        { QStringLiteral("disableNetwork"), 0 },
//        { QStringLiteral("bridges"), QStringList() }
//    };
//    torControl->setConfiguration(conf);
//    auto command = qobject_cast<shims::TorControlCommand*>(torControl->setConfiguration(conf));
//    QObject::connect(command, &shims::TorControlCommand::finished, this, &RicochetIrcServer::torConfigurationFinished);
}


void RicochetIrcServer::torConfigurationFinished()
{
    qDebug() << "==== IRC torConfigurationFinished() ====";

    auto& torControl = shims::TorControl::torControl;
    if (torControl->hasOwnership()) {
        torControl->saveConfiguration();
    }
}


void RicochetIrcServer::torRunningChanged() {
    auto& userIdentity = shims::UserIdentity::userIdentity;
    auto& torManager = shims::TorManager::torManager;
    auto& torControl = shims::TorControl::torControl;

    qDebug() << "==== IRC torRunningChanged: " << torManager->running();

    if (torManager->configurationNeeded()) {
        if (torManager->running() == "Yes") {
            QVariantMap conf = {
                { QStringLiteral("disableNetwork"), 0 },
                { QStringLiteral("bridges"), QStringList() }
            };
            torControl->setConfiguration(conf);
            torControl->saveConfiguration();
        }
    }

    if (torManager->running() == "Yes") {
        qDebug() << "=== Tor ready ===";
        getChannel(control_channel_name)->setTopic(ricochet_user, userIdentity->contactID());
    }

    if (clients.count() > 0) {
        auto contactsManager = userIdentity->getContacts();
        echo(QString("torManager->running() == " + torManager->running()));
        if (torManager->running() == "Yes") {
            // add all contacts to IRC
            foreach(shims::ContactUser* contact, contactsManager->contacts()) {
                emit onContactStatusChanged(contact, shims::ContactUser::Offline);
            }
        }
    }
}

void RicochetIrcServer::torErrorChanged() {
    qDebug() << "=== NOT-IMPLEMENTED: IRC torErrorChanged() ===";
}

void RicochetIrcServer::torLogMessage(const QString &message) {
    qDebug() << "=== NOT-IMPLEMENTED: IRC torLogMessage() ===";
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
        QString cmd = args.at(0);
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
    auto userIdentity = shims::UserIdentity::userIdentity;
    auto contactsManager = userIdentity->getContacts();
    foreach(shims::ContactUser* contact, contactsManager->contacts())
    {
        if(contact->getNickname() == contact_nick)
        {
            contact->conversation()->sendMessage(text);

            IrcUser* contact_irc_user = usermap.value(contact);
            if(contact->getStatus() != shims::ContactUser::Status::Online)
            {
                sender->reply(RPL_AWAY,
                            QStringLiteral("%1 %2 :is offline")
                            .arg(sender->nick)
                            .arg(contact->getNickname())
                            );
                // TODO
//                sender->reply(contact_irc_user->getPrefix(),
//                            QStringLiteral("PRIVMSG %1 :\x02[Contact is offline. %2 queued message(s).]")
//                            .arg(sender->getPrefix())
//                            .arg(contact->conversation()->queuedCount()));

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
    echo(QLatin1String(""));
    echo(QStringLiteral("COMMANDS:"));
    echo(QStringLiteral(" * help"));
    echo(QStringLiteral(" * id                            -- print your ricochet id"));
    echo(QStringLiteral(" * add ID NICKNAME MESSAGE       -- add a contact"));
    echo(QStringLiteral(" * delete NICKNAME               -- delete a contact"));
    echo(QStringLiteral(" * rename NICKNAME NEW_NICKNAME  -- rename a contact"));
    echo(QStringLiteral(" * request list                  -- list incoming requests"));
    echo(QStringLiteral(" * request accept ID NICKNAME    -- accept incoming request"));
    echo(QStringLiteral(" * request reject ID             -- reject incoming request"));
    echo(QLatin1String(""));
}


void RicochetIrcServer::cmdId()
{
    echo(shims::UserIdentity::userIdentity->contactID());
}


void RicochetIrcServer::cmdAdd(const QStringList& args)
{
    auto userIdentity = shims::UserIdentity::userIdentity;
    auto contactsManager = userIdentity->getContacts();

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
        shims::ContactIDValidator contactIdValidator;
        if(contactIdValidator.isValidID(id))
        {
            echo(QStringLiteral("sending contact request to user `%1` with id: %2 with message: %3").arg(nickname).arg(id).arg(message));
            // TODO: my nickname
            contactsManager->createContactRequest(id, nickname, "my nickname", message);
        }
        else
        {
            error(QStringLiteral("invalid Ricochet ID: %1").arg(id));
        }
    }
}


void RicochetIrcServer::cmdDelete(const QStringList& args)
{
    auto userIdentity = shims::UserIdentity::userIdentity;
    auto contactsManager = userIdentity->getContacts();

    if(args.length() == 1)
    {
        shims::ContactUser *contact = contactsManager->getShimContactByNickname(args[0]);
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
    auto userIdentity = shims::UserIdentity::userIdentity;
    auto contactsManager = userIdentity->getContacts();

    if(args.length() == 2)
    {
        QString old_nickname = args[0];
        QString new_nickname = args[1];

        if(contactsManager->getShimContactByNickname(new_nickname)
                || findUser(new_nickname) != Q_NULLPTR)
        {
            error(QStringLiteral("Target nickname `%1` already exists.").arg(new_nickname));
        }
        else
        {
            auto cu = contactsManager->getShimContactByNickname(old_nickname);
            if(cu)
            {
                echo(QStringLiteral("renaming user `%1` to `%2`").arg(old_nickname).arg(new_nickname));
                cu->setNickname(new_nickname);
                IrcUser* ircuser = usermap.value(cu);
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


shims::IncomingContactRequest* RicochetIrcServer::getIncomingRequestByID(const QString& id)
{
    auto userIdentity = shims::UserIdentity::userIdentity;
    foreach(QObject *obj, userIdentity->getRequests())
    {
        auto request = qobject_cast<shims::IncomingContactRequest*>(obj);
        if(request->getContactId() == id)
        {
            return request;
        }
    }
    return nullptr;
}


void RicochetIrcServer::cmdRequest(const QStringList& args)
{
    // TODO: validate arguments
    auto userIdentity = shims::UserIdentity::userIdentity;
    auto contactsManager = userIdentity->getContacts();

    if(args.length() == 1 && args[0] == QStringLiteral("list"))
    {
        echo(QStringLiteral("You have %1 incoming contact request(s):").arg(userIdentity->getRequests().count()));
        foreach(QObject *obj, userIdentity->getRequests()) {
            auto request = qobject_cast<shims::IncomingContactRequest*>(obj);
            echo(QStringLiteral("    %1 (message: %2)")
                 .arg(request->getContactId()).arg(request->getMessage()));
        }
    }
    else
    if(args.length() == 3 && args[0] == QStringLiteral("accept"))
    {
        shims::IncomingContactRequest* request = getIncomingRequestByID(args[1]);
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
        shims::IncomingContactRequest* request = getIncomingRequestByID(args[1]);
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


void RicochetIrcServer::onContactAdded(shims::ContactUser *user)
{
    connect(user->conversation(),
            &shims::ConversationModel::unreadCountChanged,
            this,
            &RicochetIrcServer::onUnreadCountChanged);
}


/**
 * @brief RicochetIrcServer::onContactStatusChanged  A contact has come online or gone offline.
 * @param user
 * @param status
 *
 * => set +v/-v in #ricochet
 * => tell the IRC user about any unsent essages
 */
void RicochetIrcServer::onContactStatusChanged(shims::ContactUser* user, int status)
{
    IrcChannel *ctrlchan = channels[control_channel_name];
    IrcUser *ircuser;

    if(ctrlchan->hasMember(user->getNickname()))
    {
        ircuser = ctrlchan->getMember(user->getNickname());
    }
    else
    {
        ircuser = new IrcUser(this);
        ircuser->nick = user->getNickname();
        ircuser->user = user->getNickname();
        ircuser->hostname = user->getContactID();
        ctrlchan->addMember(ircuser, QStringLiteral("+v"));
        joined(ircuser, control_channel_name);
        usermap.insert(user, ircuser);
        virtual_clients.append(ircuser);
    }

    switch(status)
    {
        case shims::ContactUser::Online:
            ctrlchan->setMemberFlags(ircuser, QStringLiteral("+v"));

            // TODO
            // Notify user in query when there are queued messages.
//            if(user->conversation()->queuedCount() > 0)
//            {
//                foreach(IrcConnection *conn, clients.values())
//                {
//                    conn->reply(ircuser->getPrefix(),
//                                QStringLiteral("PRIVMSG %1 :\x02[Contact is back online. Sending queued messages...]")
//                                .arg(conn->getPrefix()));
//                }
//            }
        break;
        case shims::ContactUser::Offline:
            ctrlchan->setMemberFlags(ircuser, QStringLiteral("-v"));
        break;
    }
}

/**
 * @brief RicochetIrcServer::onUnreadCountChanged  Forward Ricochet messages to IRC.
 */
void RicochetIrcServer::onUnreadCountChanged()
{
    shims::ConversationModel* convo = qobject_cast<shims::ConversationModel*>(sender());

    while(convo->rowCount())
    {
        QModelIndex index = convo->index(0);
        QString text = convo->data(index, Qt::DisplayRole).toString();
        bool isOutgoing = convo->data(index, shims::ConversationModel::IsOutgoingRole).toBool();
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
void RicochetIrcServer::requestAdded(shims::IncomingContactRequest *request)
{
    highlight();
    QString notice = QStringLiteral("Incoming contact request from: %1").arg(request->getContactId());
    if(request->getMessage().length() > 0)
    {
        notice += QStringLiteral(" (message: %1)").arg(request->getMessage());
    }
    echo(notice);
}


void RicochetIrcServer::requestRemoved(shims::IncomingContactRequest *request)
{
    qDebug() << QStringLiteral("Request removed: %1, message: %2")
         .arg(request->getContactId()).arg(request->getMessage());
}


void RicochetIrcServer::requestsChanged()
{
    qDebug() << "Contact requests changed";
}
