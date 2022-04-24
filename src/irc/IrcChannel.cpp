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
#include "IrcChannel.h"
#include "IrcUser.h"

IrcChannel::IrcChannel(QObject *parent, const QString& name) :
    QObject(parent),
    name(name),
    topic(QStringLiteral(""))
{
}

void IrcChannel::addMember(IrcUser *user, const QString& flags)
{
    members.insert(user, flags);
}

QList<IrcUser*> IrcChannel::getMembers()
{
    return members.keys();
}

void IrcChannel::setMemberFlags(IrcUser* member, const QString& flags)
{
    members[member] = flags;
    emit flagsChanged(member);
}

QString IrcChannel::getMemberFlagsLong(IrcUser *member)
{
    return members[member];
}

QString IrcChannel::getMemberFlagsShort(IrcUser *member)
{
    QString lf = getMemberFlagsLong(member);
    if(lf == QStringLiteral("+v"))
    {
        return QStringLiteral("+");
    }
    if(lf == QStringLiteral("-v"))
    {
        return QStringLiteral("");
    }
    if(lf == QStringLiteral("+o"))
    {
        return QStringLiteral("@");
    }
    if(lf == QStringLiteral("-o"))
    {
        return QStringLiteral("");
    }
    return lf;
}

QString IrcChannel::getMemberListString()
{
    QString r;
    foreach(IrcUser* member, getMembers())
    {
        r += getMemberFlagsShort(member) + member->nick + QStringLiteral(" ");
    }
    return r.trimmed();
}


bool IrcChannel::hasMember(const QString& nickname)
{
    return getMember(nickname) == NULL ? false : true;
}

IrcUser* IrcChannel::getMember(const QString& nickname)
{
    foreach(IrcUser* member, members.keys())
    {
        if(member->nick == nickname)
        {
            return member;
        }
    }
    return NULL;
}

void IrcChannel::removeMember(IrcUser* member)
{
    emit part(member);
    members.remove(member);
}

void IrcChannel::setTopic(IrcUser* sender, const QString& topic)
{
    this->topic = topic;
    emit topicChanged(sender, this);
}

const QString& IrcChannel::getTopic()
{
    return this->topic;
}
