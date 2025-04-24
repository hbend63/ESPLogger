#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>

class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(QObject *parent = nullptr);
    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead();
    void processRequest(QTcpSocket *socket, const QByteArray &requestData);

private:
    QTcpServer server;
};

#endif // HTTPSERVER_H
