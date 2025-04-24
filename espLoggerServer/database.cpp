#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

bool Database::init() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("sensordaten.db");

    if (!db.open()) {
        qDebug() << "Fehler beim Öffnen der DB:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    return query.exec("CREATE TABLE IF NOT EXISTS readings ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                      "temperature REAL, "
                      "humidity REAL)");
}

void Database::insertReading(double temperature, double humidity) {
    QSqlQuery query;
    query.prepare("INSERT INTO readings (temperature, humidity) VALUES (?, ?)");
    query.addBindValue(temperature);
    query.addBindValue(humidity);

    if (!query.exec()) {
        qDebug() << "Fehler beim Einfügen:" << query.lastError().text();
    }
}

// database.cpp
QVector<QVector<QString>> Database::selectReadings(const QString &from, const QString &to) {
    qDebug() << "Data: "<< from << "  " << to;
    QVector<QVector<QString>> results;
    QSqlQuery query;
    query.prepare("SELECT timestamp, temperature, humidity FROM readings "
                  "WHERE timestamp BETWEEN ? AND ? ORDER BY timestamp ASC");
    query.addBindValue(from);
    query.addBindValue(to);
    if (query.exec()) {
        while (query.next()) {
            QVector<QString> row;
            row << query.value(0).toString()
                << query.value(1).toString()
                << query.value(2).toString();
            results.append(row);
        }
    }
    return results;
}

QVariantMap Database::getLatestData() {
    QSqlQuery query("SELECT temperature, humidity, timestamp FROM readings ORDER BY id DESC LIMIT 1");
    QVariantMap result;
    if (query.next()) {
        result["temperature"] = query.value(0);
        result["humidity"] = query.value(1);
        result["timestamp"] = query.value(2);
    }
    return result;
}
