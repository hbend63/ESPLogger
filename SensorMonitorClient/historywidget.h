
#pragma once

#include <QWidget>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include "charthelper.h"

class HistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(QWidget *parent = nullptr);

private slots:
    void loadHistory();
    void onReplyFinished(QNetworkReply *reply);

private:
    QDateTimeEdit *fromEdit;
    QDateTimeEdit *toEdit;
    QPushButton *loadBtn;
    ChartHelper *chart;
    QNetworkAccessManager network;
};
