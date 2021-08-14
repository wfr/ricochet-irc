/* QtLocalIRCD - part of https://github.com/wfr/ricochet-irc/
 * Copyright (C) 2016, Wolfgang Frisch <wfrisch@riseup.net>
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
#ifndef IRCSERVER_H
#define IRCSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QHostAddress>
#include <QByteArray>
#include <QHash>

class IrcChannel;
class IrcConnection;
class IrcUser;

class IrcServer : public QObject
{
    Q_OBJECT
public:
    explicit IrcServer(QObject *parent = 0,
                       uint16_t port = 6667,
                       const QString& password = QStringLiteral(""));
    ~IrcServer();

    IrcChannel* getChannel(QString channel_name);

    IrcUser* findUser(const QString& nickname);

    /**
     * @brief send a raw IRC message to everyone in a channel
     * @param channel IRC channel boject
     * @param msg raw message
     * @param sender IRC user
     * @param include_sender also send to the sender themselves?
     */
    void channelMessage(IrcChannel* channel, QString msg, IrcUser* sender, bool include_sender);

    /**
     * @brief broadcast send message to EVERYONE
     * @param msg
     * @param sender
     */
    void broadcast(IrcUser* sender, QString msg);

    virtual const QString getWelcomeMessage();

signals:

public slots:
    /**
     * @brief Main entry point. invoke this to start the server.
     */
    bool run();

    void newConnection();

    void clientLoggedIn();

    /**
     * @brief IrcServer::privmsg slot for incoming PRIVMSGs; relays them to all recipients
     * @param user sender
     * @param msgtarget
     * @param text
     */
    void privmsg(IrcUser*, const QString&, const QString&);

    void joined(IrcUser*, const QString&);

    void flagsChanged(IrcUser *member);

    void rename(IrcUser* member, const QString& new_nick);

    void part(IrcConnection* conn, const QString& channel);

    void quit(IrcUser* conn);

    void disconnect(IrcConnection* conn);

    void topicChanged(IrcUser* sender, IrcChannel* channel);


protected:
    uint16_t port;
    QString password;
    QString welcome_message;

    QTcpServer *tcpServer;
    QHash<QTcpSocket*, IrcConnection*> clients;
    QList<IrcUser*> virtual_clients;
    QHash<QString, IrcChannel*> channels;

    /**
     * @brief privmsgHook called after a PRIVMSG was processed
     * Override this to implement a bot.
     */
    virtual void privmsgHook(IrcUser*, const QString&, const QString&) {}

    /**
     * @brief ircUserLoggedIn A new user connected successfully.
     * @param conn
     */
    virtual void ircUserLoggedIn(IrcConnection* conn = 0) {
        Q_UNUSED(conn);
    };

    /**
     * @brief ircUserLeft A user quit and/or disconnected
     */
    virtual void ircUserLeft(IrcConnection* conn = 0) {
        Q_UNUSED(conn);
    };

};

#endif // IRCSERVER_H
