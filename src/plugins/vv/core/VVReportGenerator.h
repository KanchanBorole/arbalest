#ifndef VVREPORTGENERATOR_H
#define VVREPORTGENERATOR_H

#include <QTreeWidget>
#include <QString>

class VVReportGenerator
{
public:
    static bool exportToCSV(const QString &path, QTreeWidget *tree, const QString &modelName);
    static bool exportToJSON(const QString &path, QTreeWidget *tree, const QString &modelName);
    static bool exportToTXT(const QString &path, QTreeWidget *tree, const QString &modelName);
};
#endif // VVREPORTGENERATOR_H
