#include "VVWidget.h"
#include <QMessageBox>
#include <QComboBox>
#include <QTextBrowser>
#include "../core/VVBackendTests.h" 
#include <include/MainWindow.h>
#include <include/Document.h>

VVWidget::VVWidget(QWidget *parent)
    : QWidget(parent)
{
    Q_INIT_RESOURCE(vv);
    layout = new QVBoxLayout(this);

    progressBar = new QProgressBar(this);
    progressBar->setFixedHeight(12);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->hide();
    progressBar->setStyleSheet("QProgressBar { border: 1px solid gray; border-radius: 5px; text-align: center; }"
                               "QProgressBar::chunk { background-color: #4CAF50; }");

    layout->addWidget(progressBar);

    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    statusLabel = new QLabel("Validation idle");

    layout->addWidget(statusLabel);

    filterDropdown = new QComboBox(this);
    filterDropdown->addItem("All", "ALL");
    filterDropdown->addItem("Errors", "ERROR");
    filterDropdown->addItem("Warnings", "WARNING");
    filterDropdown->addItem("Passed", "PASSED");
    filterDropdown->setFixedWidth(200);
    layout->addWidget(filterDropdown);

    summaryLabel = new QLabel("Errors: 0 | Warnings: 0 | Passed: 0");

    summaryLabel->setStyleSheet("font-weight: normal;");

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(5);

    headerLayout->addWidget(summaryLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(filterDropdown);
    layout->addLayout(headerLayout);

    issueTree = new QTreeWidget();

    issueTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(issueTree,
            &QTreeWidget::customContextMenuRequested,
            this,
            &VVWidget::showContextMenu);

    issueTree->setColumnCount(5);

    QStringList headers;

    headers << "Type"
            << "Test Name"
            << "Description"
            << "Issue Object"
            << "Full Path";

    issueTree->setColumnWidth(0, 80);
    issueTree->setColumnWidth(1, 150);
    issueTree->setColumnWidth(2, 200);
    issueTree->setColumnWidth(3, 100);
    issueTree->setColumnWidth(4, 300);

    issueTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    issueTree->setHeaderLabels(headers);
    issueTree->setAlternatingRowColors(true);
    issueTree->setRootIsDecorated(false);
    issueTree->setStyleSheet("QTreeWidget::item { padding: 0px; margin: 0px; }");

    layout->addWidget(issueTree);

    connect(filterDropdown, &QComboBox::currentIndexChanged, this, &VVWidget::applyFilter);

    consoleOutput = new QPlainTextEdit();
    consoleOutput->setReadOnly(true);
    consoleOutput->setMaximumHeight(150);
    layout->addWidget(consoleOutput);

    setLayout(layout);
}

void VVWidget::addIssue(
    const QString &severity,
    const QString &issue)
{
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setIcon(0, QIcon(getIconPath(severity)));
    item->setText(0, severity);
    item->setText(1, "General");
    item->setText(2, issue);
    item->setText(3, "-");
    item->setText(4, "-");

    issueTree->addTopLevelItem(item);
}

void VVWidget::addIssue(
    const QString &type,
    const QString &testName,
    const QString &description,
    const QString &objectName,
    const QString &fullPath)
{
    QTreeWidgetItem *parentItem;

    if (!objectGroups.contains(objectName))
    {
        parentItem = new QTreeWidgetItem();
        parentItem->setText(0, objectName);
        parentItem->setText(4, fullPath);

        issueTree->addTopLevelItem(parentItem);

        objectGroups[objectName] = parentItem;
    }
    else
    {
        parentItem = objectGroups[objectName];
    }

    QTreeWidgetItem *issueItem = new QTreeWidgetItem();
    issueItem->setIcon(0, QIcon(getIconPath(type)));

    issueItem->setText(0, type);
    issueItem->setText(1, testName);
    issueItem->setText(2, description);
    issueItem->setText(3, objectName);
    issueItem->setText(4, fullPath);
    parentItem->addChild(issueItem);
    parentItem->setExpanded(true);
}

void VVWidget::clearIssues()
{
    issueTree->clear();
    objectGroups.clear();
    consoleOutput->clear();
    summaryLabel->setText("Errors: 0 | Warnings: 0 | Passed: 0");
    setValidationStatus("Validation idle");
}
void VVWidget::appendConsoleMessage(const QString &message)
{
    consoleOutput->appendPlainText(message);
}

QTreeWidget *VVWidget::getIssueTree()
{
    return issueTree;
}

void VVWidget::setValidationStatus(
    const QString &status)
{
    statusLabel->setText(status);

    if (status.contains("RUNNING"))
    {
        statusLabel->setStyleSheet(
            "color: orange; "
            "font-weight: bold;");
    }
    else if (status.contains("COMPLETE"))
    {
        statusLabel->setStyleSheet(
            "color: green; "
            "font-weight: bold;");
    }
    else if (status.contains("STOPPED"))
    {
        statusLabel->setStyleSheet(
            "color: red; "
            "font-weight: bold;");
    }
    else
    {
        statusLabel->setStyleSheet(
            "color: gray;");
    }
}

void VVWidget::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = issueTree->itemAt(pos);

    if (!item)
        return;

    QString objectName = item->text(3);
    QString fullPath = item->text(4);

    QMenu menu;

    QAction *visualizeAction = menu.addAction("Visualize Object");
    QAction *detailsAction = menu.addAction("Test Result Details");
    QAction *copyAction = menu.addAction("Copy Object Path");
    QAction *selected = menu.exec(issueTree->viewport()->mapToGlobal(pos));

    if (selected == copyAction)
    {
        QApplication::clipboard()->setText(fullPath);
    }

    if (selected == detailsAction)
    {
        QTreeWidgetItem *item = issueTree->itemAt(pos);
        if (item)
        {
            QString details = item->text(2);
            QMessageBox::information(this, "Test Result Details", details);
        }
    }

    if (selected == visualizeAction)
    {
        QMessageBox::information(
            this,
            "Visualize",
            "Visualization for:\n" + objectName);
    }
}

void VVWidget::updateSummary(
    int errors,
    int warnings,
    int passed)
{
    summaryLabel->setText("Errors: " + QString::number(errors) + " | Warnings: " + QString::number(warnings) + " | Passed: " + QString::number(passed));
}

void VVWidget::applyFilter(int index)
{
    QString filterType = filterDropdown->itemData(index).toString();

    for (int i = 0; i < issueTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *parentItem = issueTree->topLevelItem(i);
        bool hasVisibleChild = false;

        for (int j = 0; j < parentItem->childCount(); ++j)
        {
            QTreeWidgetItem *child = parentItem->child(j);
            QString type = child->text(0).trimmed().toUpper();

            bool isVisible = (filterType == "ALL" || type == filterType);
            child->setHidden(!isVisible);

            if (isVisible)
                hasVisibleChild = true;
        }
        parentItem->setHidden(!hasVisibleChild);
    }
}

void VVWidget::addIssueWithDetails(const QString &type, const QString &testName, const QString &desc,
                                   const QString &objName, const QString &fullPath, const QString &rawDetails)
{
    addIssue(type, testName, desc, objName, fullPath);

    if (objectGroups.contains(objName))
    {
        QTreeWidgetItem *parentItem = objectGroups[objName];
        if (parentItem->childCount() > 0)
        {
            QTreeWidgetItem *lastItem = parentItem->child(parentItem->childCount() - 1);
            lastItem->setData(2, Qt::UserRole, rawDetails);
        }
    }
}

void VVWidget::showProgress()
{
    progressBar->setVisible(true);
}

void VVWidget::setProgress(int percentage)
{
    if (percentage > 0 && percentage < 100)
    {
        progressBar->show();
    }
    progressBar->setValue(percentage);
}

void VVWidget::hideProgress()
{
    progressBar->hide();
}

QString VVWidget::getIconPath(const QString &type)
{

    QString basePath = ":/plugins/vv/resources/icons/";

    if (type == "ERROR")
        return basePath + "error.png";
    if (type == "PASSED")
        return basePath + "passed.png";
    if (type == "WARNING")
        return basePath + "warning.png";

    return basePath + "verifyValidateIcon.png";
}

VVDocumentState VVWidget::captureState() const
{
    VVDocumentState state;

    for (int i = 0; i < issueTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *top = issueTree->topLevelItem(i);

        if (top->childCount() == 0 && top->text(3) == "-")
        {
            VVIssueRecord rec;
            rec.type = top->text(0);
            rec.description = top->text(2);
            rec.isSimple = true;
            state.issues.append(rec);
            continue;
        }

        QString objectName = top->text(0);
        QString fullPath = top->text(4);
        for (int j = 0; j < top->childCount(); ++j)
        {
            QTreeWidgetItem *child = top->child(j);
            VVIssueRecord rec;
            rec.type = child->text(0);
            rec.testName = child->text(1);
            rec.description = child->text(2);
            rec.objectName = objectName;
            rec.fullPath = fullPath;
            QVariant details = child->data(2, Qt::UserRole);
            if (details.isValid())
            {
                rec.details = details.toString();
                rec.hasDetails = true;
            }
            state.issues.append(rec);
        }
    }

    int errors = 0, warnings = 0, passed = 0;
    for (const VVIssueRecord &rec : state.issues)
    {
        if (rec.isSimple)
            continue;
        const QString t = rec.type.toUpper();
        if (t == "ERROR")
            errors++;
        else if (t == "WARNING")
            warnings++;
        else if (t == "PASSED")
            passed++;
    }

    state.consoleText = consoleOutput->toPlainText();
    state.statusText = statusLabel->text();
    state.errorCount = errors;
    state.warningCount = warnings;
    state.passCount = passed;
    state.progressValue = progressBar->value();
    state.progressVisible = progressBar->isVisible();

    return state;
}

void VVWidget::restoreState(const VVDocumentState &state)
{
    issueTree->clear();
    objectGroups.clear();
    consoleOutput->clear();

    for (const VVIssueRecord &rec : state.issues)
    {
        if (rec.isSimple)
        {
            addIssue(rec.type, rec.description);
        }
        else if (rec.hasDetails)
        {
            addIssueWithDetails(rec.type, rec.testName, rec.description, rec.objectName, rec.fullPath, rec.details);
        }
        else
        {
            addIssue(rec.type, rec.testName, rec.description, rec.objectName, rec.fullPath);
        }
    }

    consoleOutput->setPlainText(state.consoleText);
    setValidationStatus(state.statusText);
    updateSummary(state.errorCount, state.warningCount, state.passCount);

    if (state.progressVisible)
    {
        showProgress();
        setProgress(state.progressValue);
    }
    else
    {
        hideProgress();
        setProgress(0);
    }
}

void VVWidget::resetToIdle()
{
    clearIssues();
    hideProgress();
    setProgress(0);
}
