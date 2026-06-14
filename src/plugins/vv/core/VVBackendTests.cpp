#include "VVBackendTests.h"
#include <QDebug>
#include <brlcad/Database/Database.h>
#include "Document.h"
#include <CommandString/CommandString.h>
#include <string>

QMap<QString, QMap<QString, QStringList>> VVBackendTests::getDoubleGroupedTestSuites()
{
    QMap<QString, QMap<QString, QStringList>> layout;

    QMap<QString, QStringList> fileGroups;
    fileGroups["lc"] = QStringList() << "No mis-matched duplicate IDs" << "Duplicate ID check";
    fileGroups["search"] = QStringList() << "No matrices";
    fileGroups["valid title"] = QStringList() << "Valid title" << "Duplicate Geometry Names";
    layout["File"] = fileGroups;

    QMap<QString, QStringList> generalGroups;
    generalGroups["search"] = QStringList()
                              << "No nested regions" << "No empty combos" << "No solids outside of regions"
                              << "All BoTs are volume mode (should return nothing)" << "No BoTs are left hand orientation"
                              << "All regions have material" << "All regions have LOS";
    generalGroups["gqa"] = QStringList() << "No null region" << "Overlaps cleared to gridsize with tolerance";
    layout["General"] = generalGroups;

    return layout;
}

QString VVBackendTests::runMooseTest(BRLCAD::Database &db, const QString &testName, const QString &objectName)
{
    QString safeName = objectName.trimmed();
    if (safeName.isEmpty() || safeName == "_GLOBAL" || safeName.contains(" "))
        return "SKIP";

    QString quotedName = "{" + safeName + "}";
    QString cmdStr;
    if (testName == "No mis-matched duplicate IDs")
        cmdStr = "ls -c " + quotedName;
    else if (testName == "Duplicate ID check")
        cmdStr = "ls -d " + safeName;
    // else if (testName == "No null region") cmdStr = "gqa -P 1 -Ao " + safeName;
    // else if (testName == "Overlaps cleared to gridsize with tolerance") cmdStr = "gqa -P 1 -Ao -g 32mm,4mm -t 0.3mm " + safeName;
    else if (testName == "No null region")
        return "SKIP";
    else if (testName == "Overlaps cleared to gridsize with tolerance")
        return "SKIP";
    else if (testName == "No nested regions")
        cmdStr = "search " + safeName + " -type region -below -type region";
    else if (testName == "No empty combos")
        cmdStr = "search " + safeName + " -nnodes 0";
    else if (testName == "No solids outside of regions")
        cmdStr = "search " + safeName + " ! -below -type region -type shape";
    else if (testName == "All BoTs are volume mode (should return nothing)")
        cmdStr = "search " + safeName + " -type bot ! -type volume";
    else if (testName == "No BoTs are left hand orientation")
        cmdStr = "search " + safeName + " -type bot -param orient=lh";
    else if (testName == "All regions have material")
        cmdStr = "search " + safeName + " -type region ! -attr aircode ! -attr material_id";
    else if (testName == "All regions have LOS")
        cmdStr = "search " + safeName + " -type region ! -attr aircode ! -attr los";
    else if (testName == "No matrices")
        cmdStr = "search " + safeName + " ! -matrix IDN";
    else if (testName == "Valid title")
        cmdStr = "title";
    else
        return "SKIP";

    QStringList parts = cmdStr.split(' ', Qt::SkipEmptyParts);
    std::vector<std::string> stdParts;
    for (const auto &p : parts)
    {
        stdParts.push_back(p.toStdString());
    }

    std::vector<const char *> argv;
    for (const auto &part : stdParts)
    {
        argv.push_back(part.c_str());
    }
    int argc = (int)argv.size();
    argv.push_back(nullptr);

    BRLCAD::CommandString parser(db);
    BRLCAD::CommandString::State state = parser.Parse(argv.size(), argv.data());

    const char *raw = parser.Results();
    QString result = (raw != nullptr) ? QString(raw).trimmed() : "";
    parser.ClearResults();

    if (state == BRLCAD::CommandString::State::Success)
    {
        if (result.isEmpty())
        {
            return "No issues found (PASSED)";
        }
        return result;
    }
    return "Error: " + result;
}
