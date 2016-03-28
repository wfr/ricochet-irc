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


RicochetIrcServer::RicochetIrcServer(QObject *parent, uint16_t port, const QString& password, const QString& control_channel_name)
    : IrcServer(parent, port, password), control_channel_name(control_channel_name)
{
}


RicochetIrcServer::~RicochetIrcServer()
{
    delete ricochet_user;
}


void RicochetIrcServer::run()
{
    IrcServer::run();

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

    // Create the control channel #ricochet
    IrcChannel *ricochet_channel = getChannel(QStringLiteral("#ricochet"));
    ricochet_channel->addMember(ricochet_user, QStringLiteral("@"));
    channels.insert(ricochet_channel->name, ricochet_channel);

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


void RicochetIrcServer::ircUserLoggedIn(IrcConnection* conn)
{
    conn->join(QStringLiteral("#ricochet"));
}


void RicochetIrcServer::echo(const QString &text)
{
    privmsg(ricochet_user, control_channel_name, text);
}

void RicochetIrcServer::cmdHelp(const __attribute__((unused)) QStringList& args)
{
    echo(QStringLiteral("COMMANDS:"));
    echo(QStringLiteral("* help"));
    echo(QStringLiteral("* id                           -- print your own ID"));
    echo(QStringLiteral("* add ID NICKNAME MESSAGE      -- add contact"));
    echo(QStringLiteral("* rename NICKNAME NEW_NICKNAME -- rename contact"));
    echo(QStringLiteral("* request list                 -- list requests"));
    echo(QStringLiteral("* request (accept|reject) ID NICKNAME  -- manage requests"));
}

void RicochetIrcServer::cmdId(const __attribute__((unused)) QStringList& args)
{
    echo(identity->contactID());
}

void RicochetIrcServer::cmdAdd(const QStringList& args)
{
    if(args.length() != 3)
    {
        echo(QStringLiteral("INPUT ERROR: wrong number of arguments"));
    }
    else
    {
        QString id = args[0];
        QString nickname = args[1];
        QString message = args[2];
        echo(QStringLiteral("sending contact request to user `%1` with id: %2").arg(nickname).arg(id));;

        ContactsManager *contactsManager = identity->getContacts();
        contactsManager->createContactRequest(id, nickname, identity->nickname(), message);
    }
}

void RicochetIrcServer::cmdRename(const QStringList& args)
{
    if(args.length() != 2)
    {
        echo(QStringLiteral("INPUT ERROR: wrong number of arguments"));
    }
    else
    {
        QString nickname = args[0];
        QString new_nickname = args[1];
        echo(QStringLiteral("renaming user `%1` to `%2`").arg(nickname).arg(new_nickname));

        foreach(ContactUser* contactuser, usermap.keys())
        {
            if(contactuser->nickname() == nickname)
            {
                contactuser->setNickname(new_nickname);

                IrcUser* ircuser = usermap.value(contactuser);
                if(ircuser != Q_NULLPTR)
                {
                    this->rename(ircuser, new_nickname);
                }
                return;
            }
        }
    }
}

void RicochetIrcServer::cmdRequest(const QStringList& args)
{
    ContactsManager *contactsManager = identity->getContacts();
    IncomingRequestManager *irm = contactsManager->incomingRequestManager();

    if(args[0] == QStringLiteral("list"))
    {
        // list open contact requests
        foreach(IncomingContactRequest *inc, irm->requests())
        {
            echo(QStringLiteral("Contact request from: %1 with nickname: '%2' and message: '%3'")
                 .arg(inc->contactId()).arg(inc->nickname()).arg(inc->message()));
        }
    }
    else
    if(args[0] == QStringLiteral("accept"))
    {
        if(args.length() != 3)
        {
            echo(QStringLiteral("Unexpected number of arguments."));
            return;
        }
        QString subject_id = args[1];
        QString subject_nickname = args[2];
        IncomingContactRequest *subject = Q_NULLPTR;
        foreach(IncomingContactRequest *inc, irm->requests())
        {
            if(inc->contactId() == subject_id)
            {
                subject = inc;
            }
        }
        if(!subject)
        {
            echo(QStringLiteral("request not found"));
            return;
        }

        if(args[0] == QStringLiteral("accept") && subject)
        {
            subject->setNickname(subject_nickname);
            subject->accept();
        }
        else
        if(args[1] == QStringLiteral("reject") && subject)
        {
            subject->reject();
        }
    }
    else
    {
        echo(QStringLiteral("Unexpected number of arguments."));
    }
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
            cmdHelp(args);
        }
        else
        if(cmd == QStringLiteral("id"))
        {
            cmdId(args);
        }
        else
        if(cmd == QStringLiteral("add"))
        {
            cmdAdd(args);
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
            echo(QStringLiteral("INPUT ERROR: unknown command: %1").arg(cmd));
            cmdHelp(args);
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
            echo(QStringLiteral("contact `%1` is now online").arg(user->nickname()));
            ctrlchan->setMemberFlags(ircuser, QStringLiteral("+v"));
        break;
        case ContactUser::Offline:
            echo(QStringLiteral("contact `%1` is now offline").arg(user->nickname()));
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

//void RicochetIrcServer::onMessageReceived(const QString &text, const QDateTime &time, MessageId id)
//{

//}


void RicochetIrcServer::requestAdded(IncomingContactRequest *request)
{
    echo(QStringLiteral("Incoming contact request from: %1, nickname: %2, message: %3")
         .arg(request->contactId()).arg(request->nickname()).arg(request->message()));
}


void RicochetIrcServer::requestRemoved(IncomingContactRequest *request)
{
    echo(QStringLiteral("Request removed: %1, nickname: %2, message: %3")
         .arg(request->contactId()).arg(request->nickname()).arg(request->message()));
}


void RicochetIrcServer::requestsChanged()
{
    echo(QStringLiteral("Contact requests changed"));
}
