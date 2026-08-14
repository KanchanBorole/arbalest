#include "VVReportGenerator.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTreeWidget>
#include <algorithm>

struct Stats
{
    int err, warn, pass;
};
Stats getStats(QTreeWidget *tree)
{
    Stats s = {0, 0, 0};
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *p = tree->topLevelItem(i);
        for (int j = 0; j < p->childCount(); j++)
        {
            QString sev = p->child(j)->text(0).toUpper();
            if (sev == "ERROR")
                s.err++;
            else if (sev == "WARNING")
                s.warn++;
            else if (sev == "PASSED")
                s.pass++;
        }
    }
    return s;
}

static QString getModelName(QTreeWidget *tree)
{
    if (tree->topLevelItemCount() > 0)
    {
        return tree->topLevelItem(0)->text(0);
    }
    return "Unknown Model";
}

static const int kIndentStep = 5;

namespace ReportIndent
{
    static int depth = 0;
}

struct IndentGuard
{
    int amount;
    explicit IndentGuard(int amt = kIndentStep) : amount(amt) { ReportIndent::depth += amount; }
    ~IndentGuard() { ReportIndent::depth -= amount; }
};

static void writeLine(QTextStream &out, const QString &line)
{
    out << QString(ReportIndent::depth, ' ') << line << "\n";
}

static void writeBlock(QTextStream &out, const QString &block)
{
    for (const QString &l : block.split('\n'))
    {
        if (l.trimmed().isEmpty())
            continue;
        writeLine(out, l);
    }
}

static bool parseOverlapPiece(const QString &pieceIn, QString &pathA, QString &pathB,
                              QString &countVal, QString &distVal, QString &locVal)
{
    static const QRegularExpression re(
        "^(\\S+)\\s+(\\S+)\\s+count:(\\S+)\\s+dist:(\\S+)\\s+@\\s+(\\(.+\\))\\s*$");
    QRegularExpressionMatch m = re.match(pieceIn.trimmed());
    if (!m.hasMatch())
        return false;
    pathA = m.captured(1);
    pathB = m.captured(2);
    countVal = m.captured(3);
    distVal = m.captured(4);
    locVal = m.captured(5);
    return true;
}

static int writeOverlapEntries(QTextStream &out, const QString &pathVal,
                               const QString &raw, int &globalIdx)
{
    QStringList pieces;
    for (const QString &segment : raw.split('\n', Qt::SkipEmptyParts))
        for (const QString &piece : segment.split('|', Qt::SkipEmptyParts))
            if (!piece.trimmed().isEmpty())
                pieces << piece.trimmed();

    struct Row
    {
        QString a, b, count, dist, loc;
    };
    QVector<Row> rows;
    QStringList unparsed;

    for (const QString &piece : pieces)
    {
        if (piece.startsWith("Error", Qt::CaseInsensitive))
            continue;
        Row r;
        if (parseOverlapPiece(piece, r.a, r.b, r.count, r.dist, r.loc))
            rows.append(r);
        else
            unparsed << piece;
    }

    if (rows.isEmpty() && unparsed.isEmpty())
        return 0;

    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b)
              { return a.count.toInt() > b.count.toInt(); });

    int emitted = 0;
    for (const Row &r : rows)
    {
        QString idxStr = QString::number(globalIdx++).rightJustified(2, '0');
        out << "[" << idxStr << "] OVERLAP (Path: " << pathVal << "): dist: " << r.dist
            << " | count: " << r.count << " | loc: " << r.loc << "\n";
        {
            IndentGuard ig;
            writeLine(out, "A: " + r.a);
            writeLine(out, "B: " + r.b);
        }
        out << "\n";
        emitted++;
    }

    for (const QString &u : unparsed)
    {
        QString idxStr = QString::number(globalIdx++).rightJustified(2, '0');
        out << "[" << idxStr << "] OVERLAP (Path: " << pathVal << "): " << u << "\n";
        out << "\n";
        emitted++;
    }
    return emitted;
}

static QString stripTrailingDone(const QString &textIn)
{
    QStringList lines = textIn.split('\n');
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
        lines.removeLast();
    if (!lines.isEmpty() && lines.last().trimmed().compare("Done.", Qt::CaseInsensitive) == 0)
        lines.removeLast();
    return lines.join('\n');
}

static int extractEntryCount(const QString &text)
{
    static const QRegularExpression reInstances(
        "\\((\\d+)\\s*instances?\\)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reCount("count:(\\d+)");

    QRegularExpressionMatch m = reInstances.match(text);
    if (m.hasMatch())
        return m.captured(1).toInt();

    m = reCount.match(text);
    if (m.hasMatch())
        return m.captured(1).toInt();
    return 1;
}

static QString categoryLabelForTest(const QString &testName)
{
    static const QMap<QString, QString> labels = {
        {"No mis-matched duplicate IDs", "DUPLICATE REGION IDs"},
        {"Duplicate ID check", "DUPLICATE REGION IDs"},
        {"No null region", "NULL REGIONS"},
        {"Overlaps cleared to gridsize with tolerance", "GEOMETRY OVERLAPS (Cleared to gridsize with tolerance)"},
        {"Duplicate Geometry Names", "DUPLICATE GEOMETRY NAMES"},
        {"No nested regions", "NESTED REGIONS"},
        {"No empty combos", "EMPTY COMBOS"},
        {"No solids outside of regions", "SOLIDS OUTSIDE OF REGIONS"},
        {"All BoTs are volume mode (should return nothing)", "NON-VOLUME BOTs"},
        {"No BoTs are left hand orientation", "LEFT-HAND ORIENTATION BOTs"},
        {"All regions have material", "MISSING MATERIAL"},
        {"All regions have LOS", "MISSING LOS"},
        {"No regions have aircodes (except actual air regions)", "UNEXPECTED AIRCODES"},
        {"No matrices", "NON-IDENTITY MATRICES"},
        {"Valid title", "INVALID TITLE"}};
    return labels.value(testName, testName.toUpper());
}

bool VVReportGenerator::exportToTXT(const QString &path, QTreeWidget *tree, const QString &modelName)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);

    static const QVector<QString> orderedTestNames = {
        "No mis-matched duplicate IDs",
        "Duplicate ID check",
        "No matrices",
        "Valid title",
        "Duplicate Geometry Names",
        "No null region",
        "Overlaps cleared to gridsize with tolerance",
        "No nested regions",
        "No empty combos",
        "No solids outside of regions",
        "All BoTs are volume mode (should return nothing)",
        "No BoTs are left hand orientation",
        "All regions have material",
        "All regions have LOS",
        "No regions have aircodes (except actual air regions)"};

    QMap<QString, QList<QTreeWidgetItem *>> itemsByTest;
    QSet<QString> uniquePassedTests;
    int errCount = 0, warnCount = 0, passCount = 0;

    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *p = tree->topLevelItem(i);
        for (int j = 0; j < p->childCount(); j++)
        {
            QTreeWidgetItem *c = p->child(j);
            QString sev = c->text(0).toUpper();
            if (sev == "SKIP")
                continue;

            QString tn = c->text(1);
            itemsByTest[tn].append(c);

            if (sev == "ERROR")
                errCount++;
            else if (sev == "WARNING")
                warnCount++;
            else if (sev == "PASSED")
            {
                passCount++;
                uniquePassedTests.insert(tn);
            }
        }
    }

    out << "======================================================================\n";
    out << "VERIFICATION & VALIDATION REPORT\n";
    out << "======================================================================\n";
    out << QString("%1 : %2\n").arg(QString("Model Tested").leftJustified(12), modelName);
    out << QString("%1 : %2\n").arg(QString("Date").leftJustified(12), QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    out << "----------------------------------------------------------------------\n";
    out << "SUMMARY\n";
    out << "----------------------------------------------------------------------\n";
    out << "Total Issues : " << errCount << " Errors | " << warnCount << " Warnings\n";
    out << "Passed Tests : " << passCount << "\n";
    out << "\n";

    int globalIdx = 1;

    QVector<QString> testIterationOrder(orderedTestNames.begin(), orderedTestNames.end());
    for (auto it = itemsByTest.begin(); it != itemsByTest.end(); ++it)
    {
        if (!orderedTestNames.contains(it.key()))
            testIterationOrder.append(it.key());
    }

    auto processSeverity = [&](const QString &sevFilter, const QString &headingPrefix)
    {
        struct LabelGroup
        {
            QString label;
            QVector<QTreeWidgetItem *> items;
        };
        QVector<LabelGroup> labelGroups;
        QMap<QString, int> labelIndex;

        for (const QString &testName : testIterationOrder)
        {
            if (!itemsByTest.contains(testName))
                continue;

            QVector<QTreeWidgetItem *> sevItems;
            for (QTreeWidgetItem *c : itemsByTest.value(testName))
            {
                if (c->text(0).toUpper() == sevFilter)
                    sevItems.append(c);
            }
            if (sevItems.isEmpty())
                continue;

            QString label = categoryLabelForTest(testName);
            int idx;
            if (labelIndex.contains(label))
            {
                idx = labelIndex.value(label);
            }
            else
            {
                idx = labelGroups.size();
                labelIndex[label] = idx;
                labelGroups.append(LabelGroup{label, {}});
            }
            labelGroups[idx].items += sevItems;
        }

        for (const LabelGroup &lg : labelGroups)
        {
            out << "======================================================================\n";
            out << "[" << headingPrefix << "] " << lg.label << "\n";
            out << "======================================================================\n";

            QMap<QString, QList<QTreeWidgetItem *>> itemsByPath;
            for (QTreeWidgetItem *c : lg.items)
                itemsByPath[c->text(4)].append(c);

            for (auto pit = itemsByPath.begin(); pit != itemsByPath.end(); ++pit)
            {
                const QString &pathVal = pit.key();
                const QList<QTreeWidgetItem *> &group = pit.value();

                if (!group.isEmpty() && group.first()->text(1) == "Overlaps cleared to gridsize with tolerance")
                {
                    QString combinedRaw;
                    for (QTreeWidgetItem *c : group)
                    {
                        QString desc = c->text(2);
                        QString extra = c->data(2, Qt::UserRole).toString();
                        combinedRaw += (extra.isEmpty() ? desc : extra) + "\n";
                    }
                    int n = writeOverlapEntries(out, pathVal, combinedRaw, globalIdx);
                    if (n == 0)
                    {
                        QString idxStr = QString::number(globalIdx++).rightJustified(2, '0');
                        out << "[" << idxStr << "] " << group.first()->text(1) << " (Path: " << pathVal << ")\n";
                        {
                            IndentGuard ig;
                            writeLine(out, "- " + stripTrailingDone(combinedRaw));
                        }
                        out << "\n";
                    }
                    continue;
                }

                struct Entry
                {
                    QString testName, desc, extra;
                    int weight;
                };
                QVector<Entry> entries;
                for (QTreeWidgetItem *c : group)
                {
                    QString desc = stripTrailingDone(c->text(2));
                    QString extra = stripTrailingDone(c->data(2, Qt::UserRole).toString());
                    int weight = extractEntryCount(desc + "\n" + extra);
                    entries.append({c->text(1), desc, extra, weight});
                }
                std::sort(entries.begin(), entries.end(),
                          [](const Entry &a, const Entry &b)
                          { return a.weight > b.weight; });

                for (const Entry &e : entries)
                {
                    QString idxStr = QString::number(globalIdx++).rightJustified(2, '0');
                    out << "[" << idxStr << "] " << e.testName << " (Path: " << pathVal << ")\n";
                    {
                        IndentGuard ig;
                        if (!e.desc.isEmpty())
                            writeLine(out, "- " + e.desc);
                        if (!e.extra.isEmpty())
                            writeBlock(out, e.extra);
                    }
                    out << "\n";
                }
            }
        }
    };

    processSeverity("ERROR", "ERRORS");
    processSeverity("WARNING", "WARNINGS");

    QSet<QString> testsWithIssues;
    for (auto it = itemsByTest.begin(); it != itemsByTest.end(); ++it)
    {
        for (QTreeWidgetItem *c : it.value())
        {
            QString sev = c->text(0).toUpper();
            if (sev == "ERROR" || sev == "WARNING")
            {
                testsWithIssues.insert(it.key());
                break;
            }
        }
    }
    QSet<QString> trulyPassedTests = uniquePassedTests - testsWithIssues;

    out << "======================================================================\n";
    out << "PASSED TESTS\n";
    out << "======================================================================\n";

    if (trulyPassedTests.isEmpty())
    {
        out << "No checks passed.\n";
    }
    else
    {
        QStringList sortedPassed = trulyPassedTests.values();
        sortedPassed.sort();
        for (const QString &t : sortedPassed)
            out << "* " << t << "\n";
    }
    out << "\n";
    out << "Note: Any test not listed here had errors or warnings above.\n";
    out << "======================================================================\n";
    out << "End of Report.\n";

    file.close();
    return true;
}

bool VVReportGenerator::exportToCSV(const QString &path, QTreeWidget *tree, const QString &modelName)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);

    Stats st = getStats(tree);
    out << "Model," << modelName << "\n"; // Ab ye sahi chalega
    out << "Export Date," << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << "Total Errors," << st.err << "\nTotal Warnings," << st.warn << "\nTotal Passed," << st.pass << "\n\n";
    out << "Severity,Test Name,Description,Object,Path\n";

    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *p = tree->topLevelItem(i);
        for (int j = 0; j < p->childCount(); j++)
        {
            QTreeWidgetItem *c = p->child(j);

            // Details uthayein
            QString details = c->data(2, Qt::UserRole).toString();
            details.replace("\"", "\"\"");

            out << "\"" << c->text(0) << "\",\"" << c->text(1) << "\",\""
                << c->text(2) << "\",\"" << c->text(3) << "\",\""
                << c->text(4) << "\",\"" << details << "\"\n";
        }
    }
    file.close();
    return true;
}

QString escapeJson(const QString &str)
{
    QString s = str;
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    s.replace("\n", "\\n");
    s.replace("\r", "\\r");
    s.replace("\t", "\\t");
    return s;
}

bool VVReportGenerator::exportToJSON(const QString &path, QTreeWidget *tree, const QString &modelName)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);

    out << "{\n \"metadata\": {\n  \"model\": \"" << escapeJson(modelName) << "\",\n";
    out << "  \"date\": \"" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\"\n },\n";
    out << " \"results\": [\n";

    bool first = true;
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *p = tree->topLevelItem(i);
        for (int j = 0; j < p->childCount(); j++)
        {
            QTreeWidgetItem *c = p->child(j);
            if (!first)
                out << ",\n";

            QString details = c->data(2, Qt::UserRole).toString();

            out << "  { \"severity\": \"" << escapeJson(c->text(0)) << "\", "
                << "\"test\": \"" << escapeJson(c->text(1)) << "\", "
                << "\"desc\": \"" << escapeJson(c->text(2)) << "\", "
                << "\"object\": \"" << escapeJson(c->text(3)) << "\", "
                << "\"path\": \"" << escapeJson(c->text(4)) << "\", "
                << "\"details\": \"" << escapeJson(details) << "\" }";
            first = false;
        }
    }
    out << "\n ]\n}";
    file.close();
    return true;
}
