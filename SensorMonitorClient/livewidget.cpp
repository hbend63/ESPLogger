
#include "livewidget.h"
#include <QVBoxLayout>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

LiveWidget::LiveWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    tempLabel = new QLabel("Temperatur: ...", this);
    humLabel = new QLabel("Luftfeuchtigkeit: ...", this);
    layout->addWidget(tempLabel);
    layout->addWidget(humLabel);

    connect(&network, &QNetworkAccessManager::finished, this, &LiveWidget::onReplyFinished);
    connect(&timer, &QTimer::timeout, this, &LiveWidget::fetchLiveData);
    timer.start(5000);

    fetchLiveData();
}

void LiveWidget::fetchLiveData() {
    QNetworkRequest req(QUrl("http://192.168.188.50:8080/api/latest"));
    network.get(req);
    qDebug() << "get Data.";
}

void LiveWidget::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray jsonData = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();

            double temperature = obj["temperature"].toDouble();
            double humidity = obj["humidity"].toDouble();
            QString timestamp = obj["timestamp"].toString();

            qDebug() << "Temperatur:" << temperature;
            tempLabel->setText("Temperatur: " + QString::number(temperature,'g',2) + " °C");

            qDebug() << "Luftfeuchtigkeit:" << humidity;
            humLabel->setText("Luftfeuchtigkeit: " + QString::number(humidity,'g',2) + " %");
            qDebug() << "Zeitstempel:" << timestamp;
        } else {
            qDebug() << "Fehler beim Parsen:" << parseError.errorString();
        }

    }
    reply->deleteLater();
}
