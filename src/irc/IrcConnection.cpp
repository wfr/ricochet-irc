#include <QRegularExpression>
#include <QHostAddress>

#include "IrcConnection.h"
#include "IrcServer.h"
#include "IrcChannel.h"


// validate and parse raw IRC messages
static const QRegularExpression re_nickname (
        QStringLiteral("^[][\\`_^{|}A-Za-z][][\\`_^{|}A-Za-z0-9-]{0,50}$"));

static const QRegularExpression re_user (
        QStringLiteral("^[][\\`_^{|}A-Za-z][][\\`_^{|}A-Za-z0-9-]{0,50}$"));

static const QRegularExpression re_realname (
        QStringLiteral("^[A-Za-z0-9.-]+"));

static const QRegularExpression re_channel (
        QStringLiteral("^[&#+!][^\\x00\\x07\\x0a\\x0d ,:]{0,50}$"));

static const QRegularExpression re_message (
        QStringLiteral("^(?<prefix>:[^ ]+)? ?(?<command>[^ ]+) ?(?<params>.+?)(( :)(?<trailing>.*))?\r?\n?$"));


IrcConnection::IrcConnection(QObject *parent, IrcServer* server, QTcpSocket *socket)
    : IrcUser(parent), server(server), socket(socket)
{
    hostname = socket->localAddress().toString();
    buffer = new QByteArray();
    connect(socket,
            SIGNAL(readyRead()),
            SLOT(readyRead()));
    connect(socket,
            SIGNAL(disconnected()),
            SLOT(disconnected()));
    logged_in = false;
}


IrcConnection::~IrcConnection()
{
    disconnected();
}


QTcpSocket* IrcConnection::getSocket() const { return socket; }


void IrcConnection::readyRead()
{
    while (socket->bytesAvailable() > 0)
    {
        buffer->append(socket->readAll());
    }

    QString data = QString::fromUtf8(*buffer);
    buffer->clear();

    QStringList lines = data.split(QStringLiteral("\r\n"), QString::SkipEmptyParts);
    foreach(const QString& line, lines)
    {
        lineReceived(line);
    }
}


void IrcConnection::disconnected()
{
    if(buffer != Q_NULLPTR)
    {
        delete buffer;
    }
    buffer = Q_NULLPTR;
    emit disconnect(this);
}


QString IrcConnection::getLocalHostname()
{
    return socket->localAddress().toString();
}


void IrcConnection::lineReceived(const QString& line)
{
    qDebug() << "RECV:" << line;

    QRegularExpressionMatch match = re_message.match(line);
    if(!match.hasMatch())
    {
        socket->disconnect();
        qWarning() << "invalid data received, disconnecting";
        // TODO raise exception, remove connection in parent
        return;
    }

    QString prefix = match.captured(QStringLiteral("prefix"));
    QString command = match.captured(QStringLiteral("command"));
    QString trailing = match.captured(QStringLiteral("trailing"));
    QList<QString> params = match.captured(QStringLiteral("params")).split(QStringLiteral(" "));
    if(trailing != QStringLiteral(""))
    {
        params.append(trailing);
    }

    if(command == QStringLiteral("NICK"))
    {
        if(params.size() < 1)
        {
            qDebug() << "TODO: respond with error message";
        }
        else
        {
            QString new_nick = params[0];
            QRegularExpressionMatch match = re_nickname.match(new_nick);
            if(match.hasMatch())
            {
                emit rename(this, new_nick);
            }
            else
            {
                qDebug() << "TODO: respond to invalid nickname";
            }
        }
    }
    else if(command == QStringLiteral("USER"))
    {
        if(!re_user.match(params[0]).hasMatch())
        {
            qDebug() << "invalid user name";
            socket->disconnect();
            return;
        }
        user = params[0];

        if(!re_realname.match(params[3]).hasMatch())
        {
            qDebug() << "invalid real name";
            socket->disconnect();
            return;
        }
        realname = params[3];

        logged_in = true;
        emit loggedIn();

        reply(RPL_WELCOME, QStringLiteral("%1 :%2")
              .arg(nick)
              .arg(server->getWelcomeMessage()));
        reply(RPL_YOURHOST, QStringLiteral("%1 :Your host is: %2")
              .arg(nick)
              .arg(getLocalHostname()));
    }
    else if(command == QStringLiteral("JOIN"))
    {
        join(params[0]);
    }
    else if(command == QStringLiteral("PING"))
    {
        reply(QStringLiteral("PONG %1 :%2").arg(getLocalHostname()).arg(getLocalHostname()));
    }
    else if(command == QStringLiteral("PRIVMSG"))
    {
        emit privmsg(this, params[0], params[1]);
    }
    else if(command == QStringLiteral("PART"))
    {
        if(re_channel.match(params[0]).hasMatch())
        {
            emit part(this, params[0]);
        }
        else
        {
            qDebug() << "invalid real name";
            socket->disconnect();
            return;
        }
    }
    else if(command == QStringLiteral("QUIT"))
    {
        emit quit(this);
    }
    else
    {
        qDebug() << "unknown command:" << command;
    }
}


void IrcConnection::reply(const QString& text)
{
    reply(getLocalHostname(), text);
}

void IrcConnection::reply(const QString& prefix, const QString& text)
{
    QString out_str;
    QByteArray out_bytes;

    out_str = QStringLiteral(":%1 %2\r\n").arg(prefix).arg(text);
    out_bytes = out_str.toUtf8();
    socket->write(out_bytes.constData(), out_bytes.length());
    socket->flush();
    qDebug() << "SEND:" << out_str;
}


void IrcConnection::reply(IrcReplies command, const QString& text)
{
    QString cmd;
    cmd.sprintf("%03d", command);
    reply(QStringLiteral("%1 %2").arg(cmd).arg(text));
}


void IrcConnection::join(const QString& channel_name)
{
    QRegularExpressionMatch match = re_channel.match(channel_name);
    if(match.hasMatch())
    {
        IrcChannel *channel = ircserver->getChannel(channel_name);
        if(!channel->hasMember(this->nick))
        {
            channel->addMember(this);
            emit joined(this, channel_name);
        }

        QString members_str;
        foreach(IrcUser* member, channel->getMembers())
        {
            members_str += channel->getMemberFlagsShort(member) + member->nick + QStringLiteral(" ");
        }
        members_str = members_str.trimmed();

        reply(QStringLiteral("MODE %1 +ns").arg(channel_name));

        // TODO: support topics
        reply(RPL_NOTOPIC, QStringLiteral("%1 %2 :%3").arg(nick).arg(channel_name).arg(QStringLiteral("No topic is set")));
        reply(RPL_NAMREPLY, QStringLiteral("%1 @ %2 :%3").arg(nick).arg(channel_name).arg(members_str));
        reply(RPL_ENDOFNAMES, QStringLiteral("%1 %2 :%3").arg(nick).arg(channel_name).arg(QStringLiteral("End of names list")));
    }
    else
    {
        reply(ERR_NOSUCHCHANNEL, QStringLiteral("%1 %2 :No such channel").arg(nick).arg(channel_name));
    }
}


IrcServer* IrcConnection::getIrcServer()
{
    return server;
}
