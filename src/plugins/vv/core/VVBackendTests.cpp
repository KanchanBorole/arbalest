#include "VVBackendTests.h"
#include <QDebug>
#include <brlcad/Database/Database.h>
#include "Document.h"
#include <brlcad/CommandString/CommandString.h>
#include <string>

enum class ArgType
{
    ObjectName,
    ObjectPath,
    None
};

struct Arg
{
    QString value;
    ArgType type;
    Arg(QString v, ArgType t = ArgType::None) : value(v), type(t) {}
};

static QString executeCommand(BRLCAD::Database &db, const QList<Arg> &args, const QString &fullPath)
{
    std::vector<std::string> argv_str;
    for (const auto &arg : args)
    {
        if (arg.type == ArgType::ObjectName)
        {
            argv_str.push_back(fullPath.section('/', -1).toStdString());
        }
        else if (arg.type == ArgType::ObjectPath)
        {
            argv_str.push_back(fullPath.toStdString());
        }
        else
        {
            argv_str.push_back(arg.value.toStdString());
        }
    }

    std::vector<const char *> argv;
    for (const auto &s : argv_str)
        argv.push_back(s.c_str());

    BRLCAD::CommandString parser(db);
    parser.Parse((int)argv.size(), argv.data());
    QString output = parser.Results() ? QString(parser.Results()).trimmed() : "";
    parser.ClearResults();
    return output;
}

QMap<QString, QMap<QString, QStringList>> VVBackendTests::getDoubleGroupedTestSuites()
{
    return {
        {"File", {{"lc", {"No mis-matched duplicate IDs", "Duplicate ID check"}}, {"search", {"No matrices"}}, {"title", {"Valid title", "Duplicate Geometry Names"}}}},
        {"General", {{"search", {"No nested regions", "No empty combos", "No solids outside of regions", "All BoTs are volume mode (should return nothing)", "No BoTs are left hand orientation", "All regions have material", "All regions have LOS", "No regions have aircodes (except actual air regions)"}}, {"gqa", {"No null region", "Overlaps cleared to gridsize with tolerance"}}}}};
}

QString VVBackendTests::runMooseTest(BRLCAD::Database &db, const QString &testName, const QString &fullPath)
{
    if (fullPath.isEmpty() || fullPath.contains("_GLOBAL", Qt::CaseInsensitive))
        return "SKIP";
    QString result;

    if (testName == "No mis-matched duplicate IDs")
        result = executeCommand(db, {Arg("lc"), Arg("-m"), Arg("$OBJECT", ArgType::ObjectName)}, fullPath);
    else if (testName == "Duplicate ID check")
        result = executeCommand(db, {Arg("lc"), Arg("-d"), Arg("$OBJECT", ArgType::ObjectName)}, fullPath);
    else if (testName == "No null region")
    {
        result = executeCommand(db, {Arg("gqa"), Arg("-Ao"), Arg("-g", ArgType::None), Arg("32mm,4mm", ArgType::None), Arg("-t", ArgType::None), Arg("0.3mm", ArgType::None), Arg("$OBJECT", ArgType::ObjectPath)}, fullPath);
        if (result.contains("null", Qt::CaseInsensitive))
            return "Error: Null regions detected: " + result;
        return "No issues found (PASSED)";
    }
    else if (testName == "Overlaps cleared to gridsize with tolerance")
    {
        result = executeCommand(db, {Arg("gqa"), Arg("-Ao"), Arg("-g", ArgType::None), Arg("32mm,4mm", ArgType::None), Arg("-t", ArgType::None), Arg("0.3mm", ArgType::None), Arg("$OBJECT", ArgType::ObjectPath)}, fullPath);
        if (result.contains("No Overlaps", Qt::CaseInsensitive))
        {
            return "No issues found (PASSED)";
        }
        else
        {
            return "Error: Overlaps detected: " + result;
        }
    }
    else if (testName == "Duplicate Geometry Names")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-duplicate")}, fullPath);
    else if (testName == "No nested regions")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-type region -below -type region")}, fullPath);
    else if (testName == "No empty combos")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-nnodes 0")}, fullPath);
    else if (testName == "No solids outside of regions")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("! -below -type region -type shape")}, fullPath);
    else if (testName == "All BoTs are volume mode (should return nothing)")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-type bot ! -type volume")}, fullPath);
    else if (testName == "No BoTs are left hand orientation")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-type bot -param orient=lh")}, fullPath);
    else if (testName == "All regions have material")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-type region ! -attr aircode ! -attr material_id")}, fullPath);
    else if (testName == "All regions have LOS")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-type region ! -attr aircode ! -attr los")}, fullPath);
    else if (testName == "No regions have aircodes (except actual air regions)")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("-type region -attr aircode")}, fullPath);
    else if (testName == "No matrices")
        result = executeCommand(db, {Arg("search"), Arg("$OBJECT", ArgType::ObjectPath), Arg("! -matrix IDN")}, fullPath);
    else if (testName == "Valid title")
    {
        result = executeCommand(db, {Arg("title")}, fullPath);

        if (!result.isEmpty())
            return "No issues found (PASSED)";
        else
            return "Error: No title found";
    }
    else
        return "SKIP";

    if (result.isEmpty() ||
        result.contains("No issues", Qt::CaseInsensitive) ||
        result.contains("No duplicate", Qt::CaseInsensitive) ||
        result.contains("No Overlaps", Qt::CaseInsensitive) ||
        result.contains("List length: 0", Qt::CaseInsensitive))
    {
        return "No issues found (PASSED)";
    }
    return result;
}
