#include <QCoreApplication>
#include "ExampleIrcServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qSetMessagePattern(QString::fromLatin1("%{file}(%{line}): %{message}"));

    ExampleIrcServer *ircServer = new ExampleIrcServer(&a);
    QMetaObject::invokeMethod( ircServer, "run", Qt::QueuedConnection );

    return a.exec();
}
