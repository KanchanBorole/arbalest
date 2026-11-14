#include "VVController.h"

#include <QAction>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QTextStream>
#include <QToolButton>
#include <QTreeWidget>

#include <brlcad/Database/Database.h>

#include "core/VVBackendTests.h"
#include "ui/VVIssueDialog.h"
#include "ui/VVTestSelectionDialog.h"
#include "ui/VVWidget.h"

VVController::VVController(
    const VVPlugin::Host &host) : QObject(),
                                  m_host(host),
                                  m_parentWidget(host.parent),
                                  vvWidget(nullptr),
                                  vvTimer(nullptr),
                                  vvRunning(false),
                                  vvCurrentIndex(0)
{

    vvTimer = new QTimer(this);

    connect(vvTimer,
            &QTimer::timeout,
            this,
            &VVController::processVVValidation);
}

QWidget *VVController::CreatePanel(void)
{
    vvWidget = new VVWidget(m_parentWidget);

    connect(vvWidget,
            &VVWidget::geometrySelected,
            this,
            [this](const QString &name)
            {
                m_host.selectObject(name);
            });

    connect(vvWidget,
            &VVWidget::issueDoubleClicked,
            this,
            [this](QString issueText)
            {
                VVIssueDialog *dialog =
                    new VVIssueDialog(
                        issueText,
                        m_parentWidget);

                dialog->exec();

                if (m_host.activeDatabase() == nullptr)
                    return;

                const QStringList paths = m_host.topObjectPaths();

                for (const QString &path : paths)
                {
                    QString objectName = path.section('/', -1);

                    if (issueText.contains(objectName))
                    {
                        m_host.selectObject(objectName);

                        break;
                    }
                }
            });

    return vvWidget;
}

void VVController::CreateMenu(QMenuBar *menuBar)
{
    QMenu *vvMenu = menuBar->addMenu(tr("&Verification && Validation"));

    QAction *runVVAct = new QAction(tr("Run Validation"), this);

    connect(runVVAct,
            &QAction::triggered,
            this,
            &VVController::startVVValidation);
    vvMenu->addAction(runVVAct);

    QAction *createTestAct = new QAction(tr("Create new test"), this);

    connect(createTestAct,
            &QAction::triggered,
            this,
            [this]()
            {
                bool ok;

                QString testName =
                    QInputDialog::getText(
                        m_parentWidget,
                        "Create VV Test",
                        "Test name:",
                        QLineEdit::Normal,
                        "",
                        &ok);

                if (!ok || testName.isEmpty())
                    return;

                vvTests.append(testName);

                vvWidget->addIssue(
                    "INFO",
                    "Created VV test: " + testName);

                vvWidget->appendConsoleMessage(
                    "VV test added: " + testName);
            });

    vvMenu->addAction(createTestAct);

    QAction *removeTestAct =
        new QAction(
            tr("Remove test"),
            this);

    connect(removeTestAct,
            &QAction::triggered,
            this,
            [this]()
            {
                if (vvTests.isEmpty())
                {
                    vvWidget->addIssue(
                        "WARNING",
                        "No VV tests available");

                    return;
                }

                bool ok;

                QString selectedTest =
                    QInputDialog::getItem(
                        m_parentWidget,
                        "Remove VV Test",
                        "Select test:",
                        vvTests,
                        0,
                        false,
                        &ok);

                if (!ok || selectedTest.isEmpty())
                    return;

                vvTests.removeAll(selectedTest);

                vvWidget->addIssue(
                    "INFO",
                    "Removed VV test: " + selectedTest);

                vvWidget->appendConsoleMessage(
                    "VV test removed: " + selectedTest);
            });

    vvMenu->addAction(removeTestAct);

    QAction *createSuiteAct = new QAction(tr("Create test suite"), this);

    connect(createSuiteAct,
            &QAction::triggered,
            this,
            [this]()
            {
                if (vvTests.isEmpty())
                {
                    vvWidget->addIssue(
                        "WARNING",
                        "No VV tests available");

                    return;
                }

                bool ok;

                QString suiteName =
                    QInputDialog::getText(
                        m_parentWidget,
                        "Create VV Suite",
                        "Suite name:",
                        QLineEdit::Normal,
                        "",
                        &ok);

                if (!ok || suiteName.isEmpty())
                    return;

                vvSuites[suiteName] = vvTests;

                vvWidget->addIssue(
                    "INFO",
                    "Created VV suite: " + suiteName);

                vvWidget->appendConsoleMessage(
                    "VV suite created: " + suiteName);
            });

    vvMenu->addAction(createSuiteAct);

    QAction *removeSuiteAct = new QAction(tr("Remove test suite"), this);

    connect(removeSuiteAct,
            &QAction::triggered,
            this,
            [this]()
            {
                if (vvSuites.isEmpty())
                {
                    vvWidget->addIssue(
                        "WARNING",
                        "No VV suites available");

                    return;
                }

                bool ok;

                QStringList suiteNames =
                    vvSuites.keys();

                QString selectedSuite =
                    QInputDialog::getItem(
                        m_parentWidget,
                        "Remove VV Suite",
                        "Select suite:",
                        suiteNames,
                        0,
                        false,
                        &ok);

                if (!ok || selectedSuite.isEmpty())
                    return;

                vvSuites.remove(selectedSuite);

                vvWidget->addIssue(
                    "INFO",
                    "Removed VV suite: " + selectedSuite);

                vvWidget->appendConsoleMessage(
                    "VV suite removed: " + selectedSuite);
            });

    vvMenu->addAction(removeSuiteAct);

    QMenu *exportMenu = new QMenu("Export", m_parentWidget);
    QAction *exportTXTAct = new QAction("TXT", this);
    QAction *exportJSONAct = new QAction("JSON", this);
    QAction *exportCSVAct = new QAction("CSV", this);

    exportMenu->addAction(exportTXTAct);
    exportMenu->addAction(exportJSONAct);
    exportMenu->addAction(exportCSVAct);

    vvMenu->addMenu(exportMenu);
    // csv export
    connect(exportCSVAct,
            &QAction::triggered,
            this,
            [this]()
            {
                QString fileName =
                    QFileDialog::getSaveFileName(
                        m_parentWidget,
                        "Export VV Report",
                        "",
                        "CSV Files (*.csv)");

                if (fileName.isEmpty())
                    return;

                QFile file(fileName);

                if (!file.open(
                        QIODevice::WriteOnly | QIODevice::Text))
                {
                    vvWidget->addIssue(
                        "ERROR",
                        "Failed to export CSV");

                    return;
                }

                QTextStream out(&file);

                out << "Severity,Issue\n";

                for (int i = 0;
                     i < vvWidget->getIssueTree()
                             ->topLevelItemCount();
                     i++)
                {
                    QTreeWidgetItem *item = vvWidget->getIssueTree()->topLevelItem(i);

                    QString severity = item->text(0);
                    QString testName = item->text(1);
                    QString description = item->text(2);
                    QString objectName = item->text(3);
                    QString fullPath = item->text(4);

                    out << severity << ","
                        << testName << ","
                        << description << ","
                        << objectName << ","
                        << fullPath << "\n";
                }
                file.close();

                vvWidget->appendConsoleMessage("CSV report exported: " + fileName);

                vvWidget->addIssue(
                    "INFO",
                    "CSV report exported");
            });

    // txt export
    connect(exportTXTAct,
            &QAction::triggered,
            this,
            [this]()
            {
                QString fileName =
                    QFileDialog::getSaveFileName(
                        m_parentWidget,
                        "Export TXT Report",
                        "",
                        "Text Files (*.txt)");

                if (fileName.isEmpty())
                    return;

                QFile file(fileName);

                if (!file.open(
                        QIODevice::WriteOnly | QIODevice::Text))
                {
                    vvWidget->addIssue(
                        "ERROR",
                        "Failed to export TXT");

                    return;
                }

                QTextStream out(&file);

                out << "Verification & Validation Report\n";
                out << "================================\n\n";

                for (int i = 0;
                     i < vvWidget->getIssueTree()
                             ->topLevelItemCount();
                     i++)
                {
                    QTreeWidgetItem *item =
                        vvWidget->getIssueTree()
                            ->topLevelItem(i);

                    out << "Severity: "
                        << item->text(0) << "\n";

                    out << "Test Name: "
                        << item->text(1) << "\n";

                    out << "Description: "
                        << item->text(2) << "\n";

                    out << "Object: "
                        << item->text(3) << "\n";

                    out << "Full Path: "
                        << item->text(4) << "\n";

                    out << "--------------------------------\n";
                }

                file.close();

                vvWidget->appendConsoleMessage(
                    "TXT report exported: " + fileName);

                vvWidget->addIssue(
                    "INFO",
                    "TXT report exported");
            });

    // json export
    connect(exportJSONAct,
            &QAction::triggered,
            this,
            [this]()
            {
                QString fileName =
                    QFileDialog::getSaveFileName(
                        m_parentWidget,
                        "Export JSON Report",
                        "",
                        "JSON Files (*.json)");

                if (fileName.isEmpty())
                    return;

                QFile file(fileName);

                if (!file.open(
                        QIODevice::WriteOnly | QIODevice::Text))
                {
                    vvWidget->addIssue(
                        "ERROR",
                        "Failed to export JSON");

                    return;
                }

                QTextStream out(&file);

                out << "{\n";
                out << " \"validation_results\": [\n";

                for (int i = 0;
                     i < vvWidget->getIssueTree()
                             ->topLevelItemCount();
                     i++)
                {
                    QTreeWidgetItem *item = vvWidget->getIssueTree()->topLevelItem(i);

                    out << " {\n";

                    out << " \"severity\": \""
                        << item->text(0)
                        << "\",\n";

                    out << " \"test_name\": \""
                        << item->text(1)
                        << "\",\n";

                    out << " \"description\": \""
                        << item->text(2)
                        << "\",\n";

                    out << " \"issue_object\": \""
                        << item->text(3)
                        << "\",\n";

                    out << " \"full_path\": \""
                        << item->text(4)
                        << "\"\n";

                    out << " }";

                    if (i != vvWidget->getIssueTree()
                                     ->topLevelItemCount() -
                                 1)
                    {
                        out << ",";
                    }
                    out << "\n";
                }

                out << " ]\n";
                out << "}\n";

                file.close();

                vvWidget->appendConsoleMessage("JSON report exported: " + fileName);

                vvWidget->addIssue(
                    "INFO",
                    "JSON report exported");
            });
}

QList<QWidget *> VVController::CreateToolbarButtons(QWidget *parent)
{
    QList<QWidget *> buttons;

    QAction *runVVToolbarAct = new QAction("Run V&V", this);

    connect(runVVToolbarAct,
            &QAction::triggered,
            this,
            &VVController::startVVValidation);

    QToolButton *runVVButton = new QToolButton(parent);

    runVVButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    runVVButton->setDefaultAction(runVVToolbarAct);
    runVVButton->setObjectName("toolbarButton");

    buttons.append(runVVButton);

    QAction *stopVVAct = new QAction("Stop", this);

    connect(stopVVAct,
            &QAction::triggered,
            this,
            &VVController::stopVVValidation);

    QToolButton *stopVVButton = new QToolButton(parent);

    stopVVButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    stopVVButton->setDefaultAction(stopVVAct);
    stopVVButton->setObjectName("toolbarButton");

    buttons.append(stopVVButton);

    return buttons;
}

void VVController::Reset(void)
{
    if (vvWidget)
    {
        vvWidget->clearIssues();

        vvValidationQueue.clear();
        vvCurrentIndex = 0;
        vvRunning = false;

        vvTimer->stop();

        vvWidget->setValidationStatus("Validation idle");
        vvWidget->updateSummary(0, 0, 0);
    }
}

void VVController::startVVValidation()
{
    VVTestSelectionDialog dialog(m_parentWidget);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    vvTests = dialog.getSelectedTests();
    if (vvTests.isEmpty())
    {
        vvWidget->appendConsoleMessage("[WARNING] No validation tests selected.");
        vvWidget->addIssue("WARNING", "No validation tests selected");
        return;
    }

    vvWidget->appendConsoleMessage("[INFO] Selected Tests:");
    for (const QString &test : vvTests)
    {
        vvWidget->appendConsoleMessage(" - " + test);
    }

    if (!vvWidget)
        return;

    vvWidget->clearIssues();
    vvValidationQueue.clear();
    vvCurrentIndex = 0;

    if (m_host.activeDatabase() == nullptr)
    {
        vvWidget->addIssue("ERROR", "No geometry loaded");
        return;
    }

    const QStringList paths = m_host.topObjectPaths();
    for (const QString &path : paths)
    {
        if (!vvValidationQueue.contains(path))
        {
            vvValidationQueue.append(path);
        }
    }
    vvRunning = true;
    vvWidget->setValidationStatus("VALIDATION RUNNING");
    vvTimer->start(1000);
}

void VVController::processVVValidation()
{
    static int passCount = 0;
    static int warningCount = 0;
    static int errorCount = 0;

    if (vvCurrentIndex == 0)
    {
        passCount = 0;
        warningCount = 0;
        errorCount = 0;
        vvWidget->appendConsoleMessage("[INFO] Starting validation...");
    }

    if (!vvRunning)
        return;

    if (vvCurrentIndex < vvValidationQueue.size())
    {
        QString fullPath = vvValidationQueue[vvCurrentIndex];
        QString objectName = fullPath.section('/', -1);

        if (fullPath.isEmpty())
        {
            vvWidget->addIssue("ERROR", "Name Empty");
            errorCount++;
            vvCurrentIndex++;
            return;
        }
        if (fullPath.contains("_GLOBAL", Qt::CaseInsensitive))
        {
            vvCurrentIndex++;
            processVVValidation();
            return;
        }
        if (objectName.toLower().contains("temp"))
        {
            vvWidget->addIssue("WARNING", "Temp Geometry found", "N/A", objectName, fullPath);
            warningCount++;
        }

        BRLCAD::Database *db = m_host.activeDatabase();
        if (db)
        {
            if (!db)
            {
                vvWidget->appendConsoleMessage("[ERROR] Database unavailable.");
                vvCurrentIndex++;
                return;
            }

            vvWidget->appendConsoleMessage("[CHECKING] " + fullPath);

            for (const QString &testName : vvTests)
            {
                QString mooseResult = VVBackendTests::runMooseTest(*db, testName, fullPath);

                if (mooseResult == "SKIP")
                {
                    vvWidget->appendConsoleMessage("[SKIP] " + testName);
                }
                else if (mooseResult == "No issues found (PASSED)")
                {
                    passCount++;
                    vvWidget->addIssue("PASSED", testName, "No issues found", objectName, fullPath);
                    vvWidget->appendConsoleMessage("[PASSED] " + testName);
                }
                else
                {
                    errorCount++;
                    vvWidget->addIssue("ERROR", testName, mooseResult, objectName, fullPath);
                    vvWidget->appendConsoleMessage("[ERROR] " + testName + ": " + mooseResult);
                }
                QCoreApplication::processEvents();
            }
        }
        vvWidget->appendConsoleMessage("Checked object: " + fullPath);
        vvCurrentIndex++;
    }

    if (vvCurrentIndex >= vvValidationQueue.size())
    {
        vvTimer->stop();
        vvRunning = false;
        vvWidget->updateSummary(errorCount, warningCount, passCount);
        vvWidget->setValidationStatus("VALIDATION COMPLETE");
        vvWidget->appendConsoleMessage("[INFO] Validation Completed Successfully.");
    }
}

void VVController::stopVVValidation()
{
    vvTimer->stop();
    vvRunning = false;

    vvWidget->addIssue(
        "INFO",
        "Validation stopped");

    vvWidget->setValidationStatus("VALIDATION STOPPED");
}
