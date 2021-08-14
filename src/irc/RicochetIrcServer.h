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
#ifndef RICOCHETIRCSERVER_H
#define RICOCHETIRCSERVER_H

#include <QObject>
#include <QHash>
#include "IrcServer.h"

class IrcUser;

class UserIdentity;
class ContactUser;
class IncomingContactRequest;


class RicochetIrcServer : public IrcServer
{
    Q_OBJECT
public:
    explicit RicochetIrcServer(QObject *parent = 0,
                               uint16_t port = 6667,
                               const QString& password = QStringLiteral(""),
                               const QString& control_channel_name = QStringLiteral("#ricochet"));
    ~RicochetIrcServer();

public slots:
    bool run();

protected:
    void ircUserLoggedIn(IrcConnection* conn);
    void ircUserLeft(IrcConnection* conn);
    const QString getWelcomeMessage();

private slots:
    void torConfigurationNeededChanged();
    void torConfigurationFinished();
    void torStatusChanged(int newStatus, int oldStatus);

    void onContactAdded(ContactUser *user);
    void onContactStatusChanged(ContactUser* user, int status);
    void onUnreadCountChanged();
    void requestAdded(IncomingContactRequest *request);
    void requestRemoved(IncomingContactRequest *request);
    void requestsChanged();

private:
    QString control_channel_name;
    IrcUser *ricochet_user;
    QHash<ContactUser*, IrcUser*> usermap;
    UserIdentity *identity;

    void initRicochet();
    void startRicochet();
    void stopRicochet();
    bool first_start;

    void privmsgHook(IrcUser* sender, const QString& msgtarget, const QString& text);
    void handlePM(IrcConnection* sender, const QString& contact_nick, const QString& text);

    void echo(const QString& text);
    void error(const QString& text);
    void highlight();

    void cmdHelp();
    void cmdId();
    void cmdAdd(const QStringList& args);
    void cmdDelete(const QStringList& args);
    void cmdRename(const QStringList& args);
    void cmdRequest(const QStringList& args);

    IncomingContactRequest* getIncomingRequestByID(const QString& id);
};

#endif // RICOCHETIRCSERVER_H
