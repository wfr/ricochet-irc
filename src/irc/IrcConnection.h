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
