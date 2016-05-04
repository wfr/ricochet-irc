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

#ifndef IRCCHANNEL_H
#define IRCCHANNEL_H

#include <QObject>
#include <QHash>
#include <QList>

class IrcUser;

class IrcChannel : public QObject
{
    Q_OBJECT
public:
    explicit IrcChannel(QObject *parent, const QString& name);

    QString name;

    void addMember(IrcUser* user, const QString& flags = QStringLiteral(""));
    QList<IrcUser*> getMembers();
    bool hasMember(const QString& nickname);
    IrcUser* getMember(const QString& nickname);

    void setMemberFlags(IrcUser* member, const QString& flags);
    QString getMemberFlagsLong(IrcUser* member);
    QString getMemberFlagsShort(IrcUser* member);
    QString getMemberListString();

    void removeMember(IrcUser* member);

    void setTopic(IrcUser* sender, const QString& topic);
    const QString& getTopic();

signals:
    void flagsChanged(IrcUser* member);
    void part(IrcUser* member);
    void topicChanged(IrcUser* sender, IrcChannel* channel);

private:
    // {user: flags, ...}
    QHash<IrcUser*, QString> members;
    QString topic;

};

#endif // IRCCHANNEL_H
