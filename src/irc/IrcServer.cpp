/* QtLocalIRCD - part of https://github.com/wfr/ricochet-irc/
 * Copyright (C) 2016, Wolfgang Frisch <wfr@roembden.net>
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
#include "IrcServer.h"
#include "IrcUser.h"
#include "IrcChannel.h"
#include "IrcConnection.h"
#include <QTcpSocket>
#include <QTimer>
#include <QException>
#include <QDebug>

IrcServer::IrcServer(QObject *parent, uint16_t port, const QString& password)
    : QObject(parent), port(port), password(password)
{
}


IrcServer::~IrcServer()
{
    tcpServer->close();
    delete tcpServer;
    foreach(IrcConnection* client, clients.values())
    {
        delete client;
    }
    foreach(IrcChannel* channel, channels.values())
    {
        delete channel;
    }
}


bool IrcServer::run()
{
    tcpServer = new QTcpServer(this);
    if(tcpServer->listen(QHostAddress::LocalHost, port))
    {
        connect(tcpServer,
                SIGNAL(newConnection()),
                this,
                SLOT(newConnection()));
        return true;
    }
    else
    {
        qFatal("TCP listen() failed");
        return false;

    }
}


void IrcServer::newConnection()
{
    while (tcpServer->hasPendingConnections())
    {
        QTcpSocket *socket = tcpServer->nextPendingConnection();
        IrcConnection *conn = new IrcConnection(this, this, socket, password);
        clients.insert(socket, conn);
        QObject::connect(conn,
                         SIGNAL(loggedIn()),
                         this,
                         SLOT(clientLoggedIn()));
        QObject::connect(conn,
                         SIGNAL(privmsg(IrcUser*, const QString&, const QString&)),
                         this,
                         SLOT(privmsg(IrcUser*, const QString&, const QString&)));
        QObject::connect(conn,
                         SIGNAL(joined(IrcUser*, const QString&)),
                         this,
                         SLOT(joined(IrcUser*, const QString&)));
        QObject::connect(conn,
                         SIGNAL(rename(IrcUser*, const QString&)),
                         this,
                         SLOT(rename(IrcUser*, const QString&)));
        QObject::connect(conn,
                         SIGNAL(part(IrcConnection*, const QString&)),
                         this,
                         SLOT(part(IrcConnection*, const QString&)));
        QObject::connect(conn,
                         SIGNAL(quit(IrcUser*)),
                         this,
                         SLOT(quit(IrcUser*)));
        QObject::connect(conn,
                         SIGNAL(disconnect(IrcConnection*)),
                         this,
                         SLOT(disconnect(IrcConnection*)));
    }
}


void IrcServer::clientLoggedIn()
{
    IrcConnection *conn = qobject_cast<IrcConnection*>(QObject::sender());
    ircUserLoggedIn(conn);

    // Example: Force user to join a channel:
    //conn->join(QStringLiteral("#ricochet"));
}


const QString IrcServer::getWelcomeMessage()
{
    return QString(QStringLiteral("Welcome to a local IRC server!"));
}


IrcChannel* IrcServer::getChannel(QString channel_name)
{
    IrcChannel *channel = channels.value(channel_name);
    if(!channel)
    {
        channel = new IrcChannel(this, channel_name);
        channels.insert(channel_name, channel);
        QObject::connect(channel,
                         SIGNAL(flagsChanged(IrcUser*)),
                         this,
                         SLOT(flagsChanged(IrcUser*)));
        QObject::connect(channel,
                         SIGNAL(topicChanged(IrcUser*, IrcChannel*)),
                         this,
                         SLOT(topicChanged(IrcUser*, IrcChannel*)));
    }
    return channel;
}


void IrcServer::channelMessage(IrcChannel* channel, QString msg, IrcUser* sender, bool include_sender)
{
    if(channel == Q_NULLPTR)
    {
        qWarning() << "channelMessage called with NULL";
        return;
    }
    foreach(IrcUser* member, channel->getMembers())
    {
        IrcConnection* conn = qobject_cast<IrcConnection*>(member);
        if(conn != Q_NULLPTR)
        {
            if(include_sender || member->nick != sender->nick)
            {
                conn->reply(sender->getPrefix(), msg);
            }
        }
    }
}


void IrcServer::broadcast(IrcUser *sender, QString msg)
{
    foreach(IrcConnection* client, clients.values())
    {
        if(client != Q_NULLPTR)
        {
            if(client->isLoggedIn())
            {
                client->reply(sender->getPrefix(), msg);
            }
        }
        else
        {
            qWarning() << "client is NULL";
        }
    }
}


void IrcServer::privmsg(IrcUser* user, const QString& msgtarget, const QString& text)
{
    if(msgtarget[0] == QLatin1Char('#'))
    {
        QString channel_name = msgtarget;
        channelMessage(getChannel(channel_name),
                       QStringLiteral("PRIVMSG %1 :%2").arg(channel_name).arg(text),
                       user,
                       false);
    }
    else
    {
        foreach(IrcConnection* conn, clients.values())
        {
            if(conn->nick == msgtarget)
            {
                conn->reply(user->getPrefix(),
                            QStringLiteral("PRIVMSG %1 :%2").arg(msgtarget).arg(text));
            }
        }
    }
    privmsgHook(user, msgtarget, text);
}


/**
 * @brief IrcServer::joined
 * @param user sender
 * @param channel_name "#foobar"
 */
void IrcServer::joined(IrcUser* user, const QString& channel_name)
{
    channelMessage(getChannel(channel_name),
                   QStringLiteral("JOIN %1").arg(channel_name),
                   user,
                   true);
}


void IrcServer::flagsChanged(IrcUser *member)
{
    IrcChannel* channel = qobject_cast<IrcChannel*>(sender());
    QString message = QStringLiteral("MODE %1 %2 %3").arg(channel->name).arg(channel->getMemberFlagsLong(member)).arg(member->nick);
    channelMessage(channel, message, member, true);
}


void IrcServer::part(IrcConnection* conn, const QString& channel)
{
    IrcChannel* chan = getChannel(channel);
    channelMessage(chan, QStringLiteral("PART %1").arg(chan->name), conn, true);
    chan->removeMember(conn);
}


void IrcServer::quit(IrcUser* conn)
{
    broadcast(conn, QStringLiteral("QUIT :Client quit"));
    foreach(IrcChannel* chan, channels)
    {
        chan->removeMember(conn);
    }
}


void IrcServer::disconnect(IrcConnection* conn)
{
    // just in case...
    foreach(IrcChannel* chan, channels)
    {
        chan->removeMember(conn);
    }
    clients.remove(conn->getSocket());
    ircUserLeft(conn);
    conn->getSocket()->close();
}


IrcUser* IrcServer::findUser(const QString& nickname)
{
    foreach(IrcConnection* c, clients.values())
    {
        if(c->nick == nickname)
        {
            return dynamic_cast<IrcUser*>(c);
        }
    }
    foreach(IrcUser* u, virtual_clients)
    {
        if(u->nick == nickname)
        {
            return u;
        }
    }
    return Q_NULLPTR;
}


void IrcServer::rename(IrcUser *member, const QString& new_nick)
{
    IrcConnection* conn = qobject_cast<IrcConnection*>(sender());

    if(findUser(new_nick) != Q_NULLPTR)
    {
        conn->reply(ERR_NICKNAMEINUSE,
                    QStringLiteral("%1 %2 :Nickname already in use.").arg(member->nick).arg(new_nick));
        return;
    }

    broadcast(member, QStringLiteral("NICK :%1").arg(new_nick));
    member->nick = new_nick;
}


void IrcServer::topicChanged(IrcUser* sender, IrcChannel *channel)
{
    broadcast(sender, QStringLiteral("TOPIC %1 :%2").arg(channel->name).arg(channel->getTopic()));
}
