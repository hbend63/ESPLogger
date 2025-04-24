
#include "charthelper.h"
#include <QtCharts/QChart>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QDateTime>

ChartHelper::ChartHelper(QWidget *parent) : QWidget(parent) {
    tempSeries = new QLineSeries();
    humSeries = new QLineSeries();
    tempSeries->setName("Temperatur");
    humSeries->setName("Luftfeuchtigkeit");

    QChart *chart = new QChart();
    chart->addSeries(tempSeries);
    chart->addSeries(humSeries);
    chart->legend()->setVisible(true);
    chart->setTitle("Sensorverlauf");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setFormat("HH:mm:ss");
    axisX->setTitleText("Zeit");
    chart->addAxis(axisX, Qt::AlignBottom);
    tempSeries->attachAxis(axisX);
    humSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("Wert");
    chart->addAxis(axisY, Qt::AlignLeft);
    tempSeries->attachAxis(axisY);
    humSeries->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(chartView);
}

void ChartHelper::setData(const QVector<QPointF> &temperature, const QVector<QPointF> &humidity) {
    tempSeries->replace(temperature);
    humSeries->replace(humidity);
    if (!temperature.isEmpty()) {
        qreal minX = temperature.first().x();
        qreal maxX = temperature.last().x();
        chartView->chart()->axes(Qt::Horizontal).first()->setRange(QDateTime::fromMSecsSinceEpoch(minX),
                                                                   QDateTime::fromMSecsSinceEpoch(maxX));
    }

    if (!temperature.isEmpty() || !humidity.isEmpty()) {
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxY = std::numeric_limits<qreal>::min();

        for (const QPointF &p : temperature + humidity) {
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }
        chartView->chart()->axes(Qt::Vertical).first()->setRange(minY - 1, maxY + 1);  // etwas Puffer
    }

    chartView->update();  // zur Sicherheit
}
