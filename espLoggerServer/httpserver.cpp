#include "httpserver.h"
#include "database.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QUrlQuery>
#include <QByteArray>
#include <QDebug>


struct ConnectionState {
    QByteArray buffer;
    int contentLength = 0;
    bool headerParsed = false;
};

HttpServer::HttpServer(QObject *parent) : QObject(parent) {}

bool HttpServer::start(quint16 port) {
    connect(&server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
    return server.listen(QHostAddress::Any, port);
}

void HttpServer::onNewConnection() {
    QTcpSocket *socket = server.nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
}

void HttpServer::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) return;

    static QHash<QTcpSocket*, ConnectionState> states;
    ConnectionState &state = states[socket];

    state.buffer += socket->readAll();

    while (true) {
        // Header noch nicht geparst?
        if (!state.headerParsed) {
            int headerEnd = state.buffer.indexOf("\r\n\r\n");
            if (headerEnd == -1) return; // Header noch unvollständig

            QByteArray headerData = state.buffer.left(headerEnd);
            QList<QByteArray> headerLines = headerData.split('\n');

            QString requestLine = QString::fromUtf8(headerLines.first().trimmed());
            QStringList parts = requestLine.split(" ");
            if (parts.size() < 2) return;

            QString method = parts[0];
            QString path = parts[1];

            int contentLength = 0;
            for (const QByteArray &line : headerLines) {
                if (line.toLower().startsWith("content-length:")) {
                    contentLength = line.split(':')[1].trimmed().toInt();
                    break;
                }
            }

            state.contentLength = contentLength;
            state.headerParsed = true;
        }

        int headerEnd = state.buffer.indexOf("\r\n\r\n");
        int totalLength = headerEnd + 4 + state.contentLength;

        if (state.buffer.size() < totalLength) return; // Noch unvollständig

        // Ganze Anfrage ist da
        QByteArray requestData = state.buffer.left(totalLength);
        state.buffer = state.buffer.mid(totalLength);
        state.headerParsed = false;
        state.contentLength = 0;

        // Jetzt vollständige Anfrage verarbeiten
        processRequest(socket, requestData);

        // Wenn der Client die Verbindung geschlossen hat, abbrechen
        if (socket->state() != QAbstractSocket::ConnectedState) {
            states.remove(socket);
            return;
        }

        // Schleife weiter – vielleicht sind mehrere Requests angekommen
    }
}

void HttpServer::processRequest(QTcpSocket *socket, const QByteArray &requestData) {
    int headerEndIndex = requestData.indexOf("\r\n\r\n");
    QByteArray headerData = requestData.left(headerEndIndex);
    QList<QByteArray> headerLines = headerData.split('\n');

    QString requestLine = QString::fromUtf8(headerLines.first().trimmed());
    QStringList parts = requestLine.split(" ");
    QString method = parts[0];
    QString path = parts[1];

    QByteArray body = requestData.mid(headerEndIndex + 4);

    if (method == "POST" && path == "/api/data") {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            socket->write("HTTP/1.1 400 Bad JSON\r\n\r\n");
            socket->disconnectFromHost();
            return;
        }

        QJsonObject obj = doc.object();
        double temp = obj["temperature"].toDouble();
        double hum = obj["humidity"].toDouble();
        qDebug() << "storing values: " << temp << "," << hum;
        Database::insertReading(temp, hum);

        //QString response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK";

        QString response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 2\r\n"
                           "\r\n"
                           "OK";

        socket->write(response.toUtf8());
        return;
    }

    if (method == "GET" && path.startsWith("/api/export")) {
        QUrl url("http://localhost" + path);
        QUrlQuery query(url);

        QString from = query.queryItemValue("from");
        QString to = query.queryItemValue("to");

        qDebug() << "Data: " << from << " to " << to;

        if (from.isEmpty() || to.isEmpty()) {
            socket->write("HTTP/1.1 400 Bad Request\r\n\r\nMissing from or to parameter");
            return;
        }

        QVector<QVector<QString>> rows = Database::selectReadings(from, to);

        QByteArray csvData;
        QTextStream stream(&csvData);
        stream << "timestamp,temperature,humidity\n";
        for (const QVector<QString> &row : rows) {
            stream << row[0] << "," << row[1] << "," << row[2] << "\n";
        }

        QByteArray response;
        response.append("HTTP/1.1 200 OK\r\n");
        response.append("Content-Type: text/csv; charset=utf-8\r\n");
        response.append("Content-Disposition: attachment; filename=\"export.csv\"\r\n");
        response.append(QString("Content-Length: %1\r\n").arg(csvData.size()).toUtf8());
        response.append("\r\n");
        response.append(csvData);

        socket->write(response);
        return;
    }

    if (method == "GET" && path.startsWith("/api/latest"))
    {
        qDebug() << "Get latest";
        QVariantMap latest = Database::getLatestData();
        if (!latest.isEmpty()) {
            QJsonObject obj;
            obj["temperature"] = latest["temperature"].toDouble();
            obj["humidity"] = latest["humidity"].toDouble();
            obj["timestamp"] = latest["timestamp"].toString();

            QJsonDocument doc(obj);
            QByteArray json = doc.toJson();

            QByteArray response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: application/json\r\n";
            response += "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n";
            response += json;
            qDebug() << response;
            socket->write(response);
            return;
        } else {
            socket->write("HTTP/1.1 404 Not Found\r\n\r\n");
            return;
        }

    }

    if (method == "GET" && path.startsWith("/api/history"))
    {
        qDebug() << "Get history";

        QUrl url("http://localhost" + path);  // Dummy-Host zum Parsen
        QUrlQuery query(url);

        QString from = query.queryItemValue("from");
        QString to = query.queryItemValue("to");

        if (from.isEmpty() || to.isEmpty()) {
            socket->write("HTTP/1.1 400 Bad Request\r\n\r\nMissing 'from' or 'to' parameter");
            return;
        }
        qDebug() << "Params ok";
        QVector<QVector<QString>> rows = Database::selectReadings(from, to);
        if (rows.isEmpty()) {
            socket->write("HTTP/1.1 404 Not Found\r\n\r\nNo data in given range");
            return;
        }
        qDebug() << "Data ok.";
        QJsonArray jsonArray;
        for (const QVector<QString>& row : rows) {
            if (row.size() < 3) continue;  // Safety check
            QJsonObject obj;
            obj["timestamp"] = row[0];
            obj["temperature"] = row[1].toDouble();
            obj["humidity"] = row[2].toDouble();
            jsonArray.append(obj);
        }

        QJsonDocument doc(jsonArray);
        QByteArray json = doc.toJson(QJsonDocument::Compact);

        qDebug() << json;

        QByteArray response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n";
        response += json;

        socket->write(response);
        return;
    }



    socket->write("HTTP/1.1 404 Not Found\r\n\r\n");
}

