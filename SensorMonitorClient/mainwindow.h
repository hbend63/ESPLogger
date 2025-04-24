
#pragma once

#include <QMainWindow>

class QTabWidget;
class LiveWidget;
class HistoryWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    QTabWidget *tabs;
    LiveWidget *liveWidget;
    HistoryWidget *historyWidget;
};
