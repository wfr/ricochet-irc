#ifndef EXAMPLEIRCSERVER_H
#define EXAMPLEIRCSERVER_H

#include <QObject>
#include <QHash>
#include "IrcServer.h"

class IrcUser;

class ExampleIrcServer : public IrcServer
{
public:
    explicit ExampleIrcServer(QObject *parent = 0,
                              uint16_t port = 6667,
                              const QString& password = QStringLiteral(""));
public slots:
    void run();

private slots:

private:
    void privmsgHook(IrcUser* sender, const QString& msgtarget, const QString& text);
    void echo(const QString& text);
};

#endif // EXAMPLEIRCSERVER_H
