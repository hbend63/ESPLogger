
#pragma once

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

//QT_CHARTS_USE_NAMESPACE

class ChartHelper : public QWidget {
    Q_OBJECT

public:
    explicit ChartHelper(QWidget *parent = nullptr);
    void setData(const QVector<QPointF> &temperature, const QVector<QPointF> &humidity);

private:
    QChartView *chartView;
    QLineSeries *tempSeries;
    QLineSeries *humSeries;
};
