#include "VVWidget.h"
#include <QMessageBox>
#include <QComboBox>
#include <QTextBrowser>
#include "../core/VVBackendTests.h" 

VVWidget::VVWidget(QWidget *parent)
    : QWidget(parent)
{
    layout = new QVBoxLayout(this);
    titleLabel = new QLabel("Verification & Validation");
    layout->addWidget(titleLabel);

    statusLabel = new QLabel("Validation idle");

    layout->addWidget(statusLabel);

    summaryLabel = new QLabel("Errors: 0 | Warnings: 0 | Passed: 0");

    summaryLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #cccccc;");

    layout->addWidget(summaryLabel);

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

    issueTree->setHeaderLabels(headers);
    issueTree->setAlternatingRowColors(true);
    issueTree->setRootIsDecorated(false);
    layout->addWidget(issueTree);

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
    item->setText(0, severity);
    item->setText(1, "General");
    item->setText(2, issue);
    item->setText(3, "-");
    item->setText(4, "-");

    if (severity == "ERROR")
    {
        item->setBackground(
            0,
            QColor(255, 180, 180));
    }
    else if (severity == "SUCCESS")
    {
        item->setBackground(0, QColor(180, 255, 180));
    }
    else if (severity == "WARNING")
    {
        item->setBackground(
            0,
            QColor(255, 230, 180));
    }
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

    issueItem->setText(0, type);
    issueItem->setText(1, testName);
    issueItem->setText(2, description);
    issueItem->setText(3, objectName);
    issueItem->setText(4, fullPath);

    if (type == "ERROR")
    {
        issueItem->setBackground(
            0,
            QColor(255, 180, 180));
    }
    else if (type == "WARNING")
    {
        issueItem->setBackground(
            0,
            QColor(255, 230, 180));
    }
    if (type == "PASSED")
    {
        issueItem->setBackground(0, QColor(180, 255, 180));
    }
    else if (type == "ERROR")
    {
        issueItem->setBackground(0, QColor(255, 180, 180));
    }
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
    QList<QTreeWidgetItem *> selectedItems = issueTree->selectedItems();
    if (selectedItems.isEmpty())
    {
        QTreeWidgetItem *item = issueTree->itemAt(pos);
        if (!item)
            return;
        selectedItems.append(item);
    }

    QMenu menu;
    QAction *visualizeAction = menu.addAction("Visualize Object");

    QAction *detailsAction = nullptr;
    QAction *copyAction = nullptr;

    if (selectedItems.size() == 1)
    {
        detailsAction = menu.addAction("Test Result Details");
        copyAction = menu.addAction("Copy Object Path");
    }

    QAction *selected = menu.exec(issueTree->viewport()->mapToGlobal(pos));
    if (!selected)
        return;

    if (copyAction && selected == copyAction)
    {
        QApplication::clipboard()->setText(selectedItems.first()->text(4));
    }
    else if (detailsAction && selected == detailsAction)
    {
        QTreeWidgetItem *item = selectedItems.first();
        QDialog dialog(this);
        dialog.setWindowTitle("Test Result Details");
        dialog.setMinimumWidth(500);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        QTextBrowser *browser = new QTextBrowser();

        QString rawDesc = item->data(2, Qt::UserRole).toString();
        if (rawDesc.isEmpty())
        {
            rawDesc = item->text(2);
        }

        QString formattedDesc = "<pre style='font-family: monospace;'>" + rawDesc + "</pre>";
        QString htmlContent = QString(
                                  "<style>"
                                  "  body { margin: 0; padding: 0; }"
                                  "  p { margin: 0 0 8px 0; }"
                                  "  pre { margin: 0; font-family: monospace; white-space: pre-wrap; }"
                                  "</style>"
                                  "<p><b>Test Name:</b><br>%1</p>"
                                  "<p><b>Command:</b><br>%2</p>"
                                  "<p><b>Result Code:</b><br>%3</p>"
                                  "<p style='margin-bottom: 2px;'><b>Description:</b></p>"
                                  "%4")
                                  .arg(item->text(1),
                                       VVBackendTests::getCommandForTest(item->text(1)),
                                       item->text(0),
                                       formattedDesc);

        browser->setHtml(htmlContent);
        browser->setStyleSheet("border: none; background-color: transparent;");
        layout->addWidget(browser);

        QPushButton *okBtn = new QPushButton("OK");
        connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        layout->addWidget(okBtn);

        dialog.exec();
    }

    if (selected == visualizeAction)
    {
        QMessageBox::information(
            this,
            "Visualize",
            "Visualization for:\n" + objectName());
    }
}

void VVWidget::updateSummary(
    int errors,
    int warnings,
    int passed)
{
    summaryLabel->setText("Errors: " + QString::number(errors) + " | Warnings: " + QString::number(warnings) + " | Passed: " + QString::number(passed));
}
