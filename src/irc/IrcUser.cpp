#include "IrcUser.h"
#include "IrcChannel.h"
#include "IrcServer.h"

IrcUser::IrcUser(QObject *ircserver) : QObject(ircserver)
{
    this->ircserver = qobject_cast<IrcServer*>(ircserver);
}


QString IrcUser::getPrefix()
{
    return QStringLiteral("%1!~%2@%3").arg(nick, user, hostname);
}
