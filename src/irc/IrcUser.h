#ifndef IRCUSER_H
#define IRCUSER_H

#include <QObject>

class IrcServer;

class IrcUser : public QObject
{
    Q_OBJECT
public:
    explicit IrcUser(QObject *ircserver = 0);

    QString nick, user, hostname, realname;
    virtual QString getPrefix();

signals:
    void privmsg(IrcUser *user, const QString& msgtarget, const QString& text);
    void joined(IrcUser* user, const QString& channel);

public slots:

protected:
    IrcServer *ircserver;

private:
};

#endif // IRCUSER_H
