
#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>

class LiveWidget : public QWidget {
    Q_OBJECT

public:
    explicit LiveWidget(QWidget *parent = nullptr);

private slots:
    void fetchLiveData();
    void onReplyFinished(QNetworkReply *reply);

private:
    QLabel *tempLabel;
    QLabel *humLabel;
    QTimer timer;
    QNetworkAccessManager network;
};
