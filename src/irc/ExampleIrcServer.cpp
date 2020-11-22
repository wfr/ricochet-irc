#include "ExampleIrcServer.h"
#include "IrcServer.h"
#include "IrcUser.h"
#include "IrcChannel.h"
#include "IrcConnection.h"

ExampleIrcServer::ExampleIrcServer(QObject *parent, uint16_t port, const QString& password)
    : IrcServer(parent, port, password)
{
}



void ExampleIrcServer::run()
{
    IrcServer::run();
}


void ExampleIrcServer::echo(const QString &text)
{
    //privmsg(ricochet_user, control_channel_name, text);
}


void ExampleIrcServer::privmsgHook(IrcUser* sender, const QString& msgtarget, const QString& text)
{
}


