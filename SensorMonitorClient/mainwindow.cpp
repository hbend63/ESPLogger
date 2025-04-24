
#include "mainwindow.h"
#include "livewidget.h"
#include "historywidget.h"
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), tabs(new QTabWidget(this)) {

    liveWidget = new LiveWidget(this);
    historyWidget = new HistoryWidget(this);

    tabs->addTab(liveWidget, "Live-Daten");
    tabs->addTab(historyWidget, "Verlauf");

    setCentralWidget(tabs);
    setWindowTitle("Sensor Monitor Client");
    resize(800, 600);
}
