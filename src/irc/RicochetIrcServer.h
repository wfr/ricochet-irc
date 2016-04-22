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

    void echo(const QString& text);
    void error(const QString& text);

    void cmdHelp();
    void cmdId();
    void cmdAdd(const QStringList& args);
    void cmdDelete(const QStringList& args);
    void cmdRename(const QStringList& args);
    void cmdRequest(const QStringList& args);

    IncomingContactRequest* getIncomingRequestByID(const QString& id);
};

#endif // RICOCHETIRCSERVER_H
