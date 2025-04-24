
#include "historywidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonParseError>
#include <QJsonObject>

HistoryWidget::HistoryWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *controls = new QHBoxLayout;

    fromEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(-3600), this);
    toEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    fromEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    toEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

    loadBtn = new QPushButton("Laden", this);
    controls->addWidget(fromEdit);
    controls->addWidget(toEdit);
    controls->addWidget(loadBtn);

    chart = new ChartHelper(this);
    mainLayout->addLayout(controls);
    mainLayout->addWidget(chart);

    connect(loadBtn, &QPushButton::clicked, this, &HistoryWidget::loadHistory);
    connect(&network, &QNetworkAccessManager::finished, this, &HistoryWidget::onReplyFinished);
}

void HistoryWidget::loadHistory() {
    QString from = fromEdit->dateTime().toString(Qt::ISODate);
    QString to = toEdit->dateTime().toString(Qt::ISODate);
    QString url = QString("http://192.168.188.50:8080/api/history?from=%1&to=%2").arg(from, to);
    network.get(QNetworkRequest(QUrl(url)));
}

void HistoryWidget::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QVector<QPointF> temps, hums;
        QByteArray data = reply->readAll();
        qDebug() << "Got: " << data;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            qWarning() << "Fehler beim Parsen der JSON-Daten:" << parseError.errorString();
            reply->deleteLater();
            return;
        }

        QJsonArray array = doc.array();
        for (const QJsonValue &val : array) {
            if (!val.isObject()) continue;
            QJsonObject obj = val.toObject();

            QString tsStr = obj["timestamp"].toString();
            QDateTime ts = QDateTime::fromString(tsStr, "yyyy-MM-dd HH:mm:ss");
            if (!ts.isValid()) {
                qWarning() << "Ungültiger Timestamp:" << tsStr;
                continue;
            }

            double temperature = obj["temperature"].toDouble();
            double humidity = obj["humidity"].toDouble();

            qDebug() << "Out: "<< temperature << "  " << humidity;

            temps.append(QPointF(ts.toMSecsSinceEpoch(), temperature));
            hums.append(QPointF(ts.toMSecsSinceEpoch(), humidity));
        }

        chart->setData(temps, hums);
    } else {
        qWarning() << "Fehler bei Netzwerkantwort:" << reply->errorString();
    }

    reply->deleteLater();
}
