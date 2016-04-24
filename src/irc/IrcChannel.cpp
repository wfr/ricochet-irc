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
