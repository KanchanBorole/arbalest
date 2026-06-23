#include "VVBackendTests.h"
#include <QDebug>
#include <brlcad/Database/Database.h>
#include "Document.h"
#include <brlcad/CommandString/CommandString.h>
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

QString VVBackendTests::runMooseTest(BRLCAD::Database &db, const QString &testName, const QString &fullPath)
{
    QString resolvedPath = fullPath.trimmed();
    if (resolvedPath.contains("_GLOBAL", Qt::CaseInsensitive))
        return "SKIP";
    if (resolvedPath.isEmpty())
        return "SKIP";
    if (!resolvedPath.startsWith("/"))
        resolvedPath = "/" + resolvedPath;

    QString objectName = resolvedPath.section('/', -1);
    QString cmd;
    QStringList args;

    if (testName == "No mis-matched duplicate IDs")
    {
        cmd = "ls";
        args << "-c" << resolvedPath;
    }
    else if (testName == "Duplicate ID check")
    {
        cmd = "ls";
        args << "-d" << resolvedPath;
    }
    else if (testName == "Duplicate Geometry Names")
    {
        cmd = "search";
        args << resolvedPath << "-duplicate";
    }
    else if (testName == "No null region" || testName == "Overlaps cleared to gridsize with tolerance")
    {
        cmd = "gqa";
        args << "-P" << "1" << "-Ao" << "-g" << "32mm,4mm" << "-t" << "0.3mm" << resolvedPath;
    }
    else if (testName == "No nested regions")
    {
        cmd = "search";
        args << resolvedPath << "-type" << "region" << "-below" << "-type" << "region";
    }
    else if (testName == "No empty combos")
    {
        cmd = "search";
        args << resolvedPath << "-nnodes" << "0";
    }
    else if (testName == "No solids outside of regions")
    {
        cmd = "search";
        args << resolvedPath << "!" << "-below" << "-type" << "region" << "-type" << "shape";
    }
    else if (testName == "All BoTs are volume mode (should return nothing)")
    {
        cmd = "search";
        args << resolvedPath << "-type" << "bot" << "!" << "-type" << "volume";
    }
    else if (testName == "No BoTs are left hand orientation")
    {
        cmd = "search";
        args << resolvedPath << "-type" << "bot" << "-param" << "orient=lh";
    }
    else if (testName == "All regions have material")
    {
        cmd = "search";
        args << resolvedPath << "-type" << "region" << "!" << "-attr" << "aircode" << "!" << "-attr" << "material_id";
    }
    else if (testName == "All regions have LOS")
    {
        cmd = "search";
        args << resolvedPath << "-type" << "region" << "!" << "-attr" << "aircode" << "!" << "-attr" << "los";
    }
    else if (testName == "No matrices")
    {
        cmd = "search";
        args << resolvedPath << "!" << "-matrix" << "IDN";
    }
    else if (testName == "Valid title")
    {
        cmd = "title";
    }
    else
    {
        return "SKIP";
    }

    std::vector<std::string> stdParts;
    stdParts.push_back(cmd.toStdString());
    for (const auto &p : args)
        stdParts.push_back(p.toStdString());
    std::vector<const char *> argv;
    for (const auto &part : stdParts)
        argv.push_back(part.c_str());

    BRLCAD::CommandString parser(db);
    BRLCAD::CommandString::State state = parser.Parse((int)argv.size(), argv.data());

    QString result = "";
    if (parser.Results())
    {
        result = QString(parser.Results()).trimmed();
    }
    else
    {
        result = "Error: No output from BRL-CAD command";
    }
    parser.ClearResults();

    if (state != BRLCAD::CommandString::State::Success)
        return "Error: " + result;

    if (testName == "Duplicate Geometry Names")
    {
        QStringList lines = result.split('\n', Qt::SkipEmptyParts);
        QMap<QString, int> pathCounts;

        for (const QString &line : lines)
        {
            QString path = line.trimmed();
            if (!path.isEmpty())
            {
                pathCounts[path]++;
            }
        }

        for (auto it = pathCounts.begin(); it != pathCounts.end(); ++it)
        {
            if (it.value() > 1)
            {
                return "Error: Duplicate Path Found: " + it.key();
            }
        }
        return "No issues found (PASSED)";
    }
    if (testName == "Valid title")
        return (!result.isEmpty()) ? "No issues found (PASSED)" : "Error: Title empty";

    if (testName == "No null region")
    {
        if (result.contains("no geometry", Qt::CaseInsensitive) || result.contains("is null", Qt::CaseInsensitive))
        {
            return "Error: Null region detected: " + result.section('\n', 0, 0);
        }
        return "No issues found (PASSED)";
    }

    if (testName == "Overlaps cleared to gridsize with tolerance")
    {
        if (result.contains("list Overlaps:", Qt::CaseInsensitive))
        {
            if (!result.contains("No Overlaps", Qt::CaseInsensitive))
            {
                return result;
            }
        }
        return "No issues found (PASSED)";
    }
    return result.isEmpty() ? "No issues found (PASSED)" : result;
}
