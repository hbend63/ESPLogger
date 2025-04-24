#ifndef DATABASE_H
#define DATABASE_H
#include <QVector>

class Database
{
public:
    static bool init();
    static void insertReading(double temperature, double humidity);
    static QVector<QVector<QString>> selectReadings(const QString &from, const QString &to);
    static QVariantMap getLatestData();
};

#endif // DATABASE_H
