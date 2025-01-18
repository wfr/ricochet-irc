#pragma once

class RicochetIrcServer;
class QCoreApplication;

class RicochetIrcServerTask : public QObject
{
    Q_OBJECT
public:
    RicochetIrcServerTask(QCoreApplication *app);

private:
    RicochetIrcServer* irc_server;

signals:
    void finished();

public slots:
    void run();
};
