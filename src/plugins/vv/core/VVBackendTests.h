#ifndef VVBACKENDTESTS_H
#define VVBACKENDTESTS_H

#include <QString>
#include <QStringList>
#include <QMap>

namespace BRLCAD {
    class Database;
}

class VVBackendTests {
public:
    static QMap<QString, QMap<QString, QStringList>> getDoubleGroupedTestSuites();
    static QString runMooseTest(BRLCAD::Database& db, const QString& testName, const QString& objectName);
};

#endif // VVBACKENDTESTS_H
