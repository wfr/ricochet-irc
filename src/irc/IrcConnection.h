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
#ifndef IRCCONNECTION_H
#define IRCCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include "IrcUser.h"
#include "IrcConstants.h"

class IrcServer;
class IrcChannel;

class IrcConnection : public IrcUser
{
    Q_OBJECT
public:
    explicit IrcConnection(QObject *parent, IrcServer* server, QTcpSocket* socket, const QString& password = QStringLiteral(""));
    ~IrcConnection();

    QTcpSocket* getSocket() const;

    void reply(const QString& text);

    void reply(const QString& prefix, const QString& text);

    void reply(IrcReplies command, const QString& text);

    void join(const QString& channel_name);

    void messageChannel(const QString& command, const QString& text);

signals:
    void loggedIn();
    void joined(IrcUser* user, const QString& channel);
    void rename(IrcUser* user, const QString& new_nick);
    void part(IrcConnection* conn, const QString& channel);
    void quit(IrcConnection* conn);
    void disconnect(IrcConnection* conn);

public slots:
    void readyRead();
    void disconnected();

private:
    IrcServer *server;
    QTcpSocket *socket;
    QByteArray *buffer;

    QString password;

    QString getLocalHostname();
    void handleLine(const QString& line);

    bool have_nick, have_user, have_pass, welcome_sent;
    bool isLoggedIn();
    void checkLogin();

    IrcServer* getIrcServer();
};

#endif // IRCCONNECTION_H
