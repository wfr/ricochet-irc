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
    QString topic;

    void addMember(IrcUser* user, const QString& flags = QStringLiteral(""));
    QList<IrcUser*> getMembers();
    bool hasMember(const QString& nickname);
    IrcUser* getMember(const QString& nickname);

    void setMemberFlags(IrcUser* member, const QString& flags);
    QString getMemberFlags(IrcUser* member);

    void removeMember(IrcUser* member);

signals:
    void flagsChanged(IrcUser* member);
    void part(IrcUser* member);

private:
    // {user: flags, ...}
    QHash<IrcUser*, QString> members;
};

#endif // IRCCHANNEL_H
