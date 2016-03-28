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

signals:

public slots:
    void run();

protected:
    void ircUserLoggedIn(IrcConnection* conn);

private slots:
    void onContactAdded(ContactUser *user);
//    void outgoingRequestAdded(OutgoingContactRequest *request);
//    void unreadCountChanged(ContactUser *user, int unreadCount);
    void onContactStatusChanged(ContactUser* user, int status);
    void onUnreadCountChanged();
    //void onMessageReceived(const QString &text, const QDateTime &time, MessageId id);

    void requestAdded(IncomingContactRequest *request);
    void requestRemoved(IncomingContactRequest *request);
    void requestsChanged();

private:
    IrcUser *ricochet_user;
    QString control_channel_name;

    QHash<ContactUser*, IrcUser*> usermap;

    void privmsgHook(IrcUser* sender, const QString& msgtarget, const QString& text);
    void echo(const QString& text);

    UserIdentity *identity;

    void cmdHelp(const QStringList& args);
    void cmdAdd(const QStringList& args);
    void cmdId(const QStringList& args);
    void cmdRename(const QStringList& args);
    void cmdRequest(const QStringList& args);
};

#endif // RICOCHETIRCSERVER_H
