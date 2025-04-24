#include <QCoreApplication>
#include "httpserver.h"
#include "database.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (!Database::init()) {
        qCritical() << "Datenbank konnte nicht initialisiert werden.";
        return 1;
    }

    HttpServer server;
    if (!server.start(8080)) {
        qCritical() << "Server konnte nicht gestartet werden.";
        return 1;
    }

    return a.exec();
}
