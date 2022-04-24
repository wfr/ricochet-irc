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
        QStringLiteral("^(?<prefix>:[^ ]+)? ?(?<command>[^ ]+) ?(?<params>.+?)?(( :)(?<trailing>.*))?\r?\n?$"));


IrcConnection::IrcConnection(QObject *parent, IrcServer* server, QTcpSocket *socket, const QString& password)
    : IrcUser(parent),
      server(server),
      socket(socket),
      password(password),
      have_nick(false),
      have_user(false),
      have_pass(false),
      welcome_sent(false)
{
    hostname = socket->localAddress().toString();
    buffer = new QByteArray();
    configureHandlers();
    connect(socket,
            SIGNAL(readyRead()),
            SLOT(readyRead()));
    connect(socket,
            SIGNAL(disconnected()),
            SLOT(disconnected()));

    have_pass = password.count() == 0 ? true : false;
}


IrcConnection::~IrcConnection()
{
    disconnected();
}


QTcpSocket* IrcConnection::getSocket() const {
    return socket;
}


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
        handleLine(line);
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


IrcServer* IrcConnection::getIrcServer()
{
    return server;
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


bool IrcConnection::isLoggedIn()
{
    return have_nick && have_user && have_pass;
}


void IrcConnection::checkLogin()
{
    if(isLoggedIn() && !welcome_sent) {
        reply(RPL_WELCOME, QStringLiteral("%1 :%2")
              .arg(nick)
              .arg(server->getWelcomeMessage()));
        reply(RPL_YOURHOST, QStringLiteral("%1 :Your host is: %2")
              .arg(nick)
              .arg(getLocalHostname()));
        emit loggedIn();
        welcome_sent = true;
    }
}

void IrcConnection::handleLine(const QString& line)
{
    qDebug() << "RECV:" << line;
    QRegularExpressionMatch match = re_message.match(line);
    if(!match.hasMatch())
    {
        qWarning() << "invalid data received, disconnecting";
        socket->close();
        return;
    }

    QString prefix = match.captured(QStringLiteral("prefix"));
    QString command = match.captured(QStringLiteral("command"));
    QList<QString> params = match.captured(QStringLiteral("params")).split(QStringLiteral(" "));
    QString trailing = match.captured(QStringLiteral("trailing"));
    if(trailing.count())
    {
        params.append(trailing);
    }

    if(command_funcs.contains(command))
    {
        if(!have_pass && command != QStringLiteral("PASS"))
        {
            reply(ERR_NOTREGISTERED, QStringLiteral("User cannot proceed without a password."));
        }
        else
        {
            command_funcs[command](params);
        }
    }
    else
    {
        qDebug() << "unknown IRC command: " << command;
    }
}


void IrcConnection::configureHandlers()
{
    command_funcs.insert(QStringLiteral("PASS"), [this](QList<QString> params)    { this->handle_PASS(params); });
    command_funcs.insert(QStringLiteral("NICK"), [this](QList<QString> params)    { this->handle_NICK(params); });
    command_funcs.insert(QStringLiteral("USER"), [this](QList<QString> params)    { this->handle_USER(params); });
    command_funcs.insert(QStringLiteral("JOIN"), [this](QList<QString> params)    { this->handle_JOIN(params); });
    command_funcs.insert(QStringLiteral("PING"), [this](QList<QString> params)    { this->handle_PING(params); });
    command_funcs.insert(QStringLiteral("PRIVMSG"), [this](QList<QString> params) { this->handle_PRIVMSG(params); });
    command_funcs.insert(QStringLiteral("PART"), [this](QList<QString> params)    { this->handle_PART(params); });
    command_funcs.insert(QStringLiteral("QUIT"), [this](QList<QString> params)    { this->handle_QUIT(params); });
}


void IrcConnection::handle_PASS(QList<QString> params)
{
    if(params.size() < 1)
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Expected argument."));
        return;
    }
    if(!have_pass)
    {
        if(password == params[0])
        {
            have_pass = true;
        }
        else
        {
            reply(ERR_PASSWDMISMATCH, QStringLiteral("Invalid password."));
            socket->close();
            return;
        }
    }
    checkLogin();
}


void IrcConnection::handle_NICK(QList<QString> params)
{
    if(params.size() < 1)
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Expected argument."));
        return;
    }
    if(!re_nickname.match(params[0]).hasMatch())
    {
        reply(ERR_ERRONEUSNICKNAME, QStringLiteral("Invalid nickname."));
        return;
    }
    emit rename(this, params[0]);
    if(!have_nick)
    {
        have_nick = true;
        checkLogin();
    }
}


void IrcConnection::handle_USER(QList<QString> params)
{
    if(params.size() < 4)
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Expected argument."));
        return;
    }
    if(!re_user.match(params[0]).hasMatch())
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Invalid user name."));
        socket->close();
        return;
    }
    if(!re_realname.match(params[3]).hasMatch())
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Invalid real name."));
        socket->close();
        return;
    }
    this->user = params[0];
    this->realname = params[3];
    this->have_user = true;
    checkLogin();
}


void IrcConnection::handle_JOIN(QList<QString> params)
{
    if(params.size() < 1)
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Expected argument."));
        return;
    }
    if(!isLoggedIn())
    {
        reply(ERR_NOLOGIN, QStringLiteral("Access prohibited."));
        return;
    }
    join(params[0]);
}


void IrcConnection::handle_PRIVMSG(QList<QString> params)
{
    if(params.size() < 2)
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Expected argument."));
        return;
    }
    if(!isLoggedIn())
    {
        reply(ERR_NOLOGIN, QStringLiteral("Access prohibited."));
        return;
    }
    emit privmsg(this, params[0], params[1]);
}


void IrcConnection::handle_PART(QList<QString> params)
{
    if(params.size() < 1)
    {
        reply(ERR_UNKNOWNCOMMAND, QStringLiteral("Expected argument."));
        return;
    }
    if(!isLoggedIn())
    {
        reply(ERR_NOLOGIN, QStringLiteral("Access prohibited."));
        return;
    }
    if(!re_channel.match(params[0]).hasMatch())
    {
        reply(ERR_NOSUCHCHANNEL, QStringLiteral("Invalid channel name."));
        return;
    }
    emit part(this, params[0]);
}


void IrcConnection::handle_PING(QList<QString> params)
{
    (void)params;
    reply(QStringLiteral("PONG %1 :%2").arg(getLocalHostname()).arg(getLocalHostname()));
}


void IrcConnection::handle_QUIT(QList<QString> params)
{
    (void)params;
    emit quit(this);
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
            reply(QStringLiteral("MODE %1 +ns").arg(channel_name));
            if (channel->getTopic() == "") {
                reply(RPL_NOTOPIC, QStringLiteral("%1 %2 :%3").arg(nick).arg(channel_name).arg(QStringLiteral("No topic is set")));
            } else {
                reply(RPL_TOPIC, QStringLiteral("%1 %2 :%3").arg(nick).arg(channel_name).arg(channel->getTopic()));
            }
            reply(RPL_NAMREPLY, QStringLiteral("%1 @ %2 :%3").arg(nick).arg(channel_name).arg(channel->getMemberListString()));
            reply(RPL_ENDOFNAMES, QStringLiteral("%1 %2 :%3").arg(nick).arg(channel_name).arg(QStringLiteral("End of names list")));
        }

    }
    else
    {
        reply(ERR_NOSUCHCHANNEL, QStringLiteral("%1 %2 :No such channel").arg(nick).arg(channel_name));
    }
}
