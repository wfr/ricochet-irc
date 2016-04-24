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
