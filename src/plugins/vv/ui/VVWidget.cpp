#include "VVWidget.h"
#include <QMessageBox>

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

    else if (selected == visualizeAction)
    {
        if (!mainWindow)
            return;
        Document *doc = mainWindow->getActiveDocument();
        if (!doc)
            return;

        ObjectTree *tree = doc->getObjectTree();
        if (!tree)
            return;

        GeometryRenderer *renderer = doc->getGeometryRenderer();
        if (!renderer)
        {
            return;
        }

        QHash<size_t, ObjectTreeItem *> &allItems = tree->getItems();
        for (auto it = allItems.begin(); it != allItems.end(); ++it)
        {
            ObjectTreeItem *item = it.value();
            if (item && !item->isRoot())
            {
                tree->changeVisibilityState(item->getObjectId(), false);
                renderer->clearSolidIfAvailable(item->getObjectId());
            }
        }

        QSet<QString> processedIdentifiers;

        for (QTreeWidgetItem *widgetItem : selectedItems)
        {
            QString rawDetails = widgetItem->data(2, Qt::UserRole).toString();
            if (rawDetails.isEmpty())
            {
                rawDetails = widgetItem->text(2);
            }
            if (rawDetails.contains("REGION", Qt::CaseInsensitive) && rawDetails.contains("PARENT", Qt::CaseInsensitive))
            {
                QStringList lines = rawDetails.split("\n", Qt::SkipEmptyParts);

                for (const QString &line : lines)
                {
                    if (line.startsWith("ID") || line.startsWith("Done") || line.trimmed().isEmpty())
                    {
                        continue;
                    }

                    QStringList columns = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (columns.size() >= 5)
                    {
                        QString regionName = columns.at(4);
                        for (auto it = allItems.begin(); it != allItems.end(); ++it)
                        {
                            ObjectTreeItem *treeItem = it.value();
                            if (treeItem && (treeItem->getName().compare(regionName, Qt::CaseInsensitive) == 0 ||
                                             treeItem->getPath().endsWith("/" + regionName, Qt::CaseInsensitive)))
                            {
                                QString uniqueKey = treeItem->getPath();
                                if (!processedIdentifiers.contains(uniqueKey))
                                {
                                    processedIdentifiers.insert(uniqueKey);
                                    tree->changeVisibilityState(treeItem->getObjectId(), true);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                QStringList parts = rawDetails.split(' ');
                for (QString part : parts)
                {
                    part = part.trimmed();
                    if (part.startsWith("/"))
                    {
                        int spaceIdx = part.indexOf(' ');
                        if (spaceIdx != -1)
                            part = part.left(spaceIdx);
                        part = part.remove('\'').remove('"').remove(')').remove('(').trimmed();

                        for (auto it = allItems.begin(); it != allItems.end(); ++it)
                        {
                            ObjectTreeItem *treeItem = it.value();
                            if (treeItem && treeItem->getPath().compare(part, Qt::CaseInsensitive) == 0)
                            {
                                QString uniqueKey = treeItem->getPath();
                                if (!processedIdentifiers.contains(uniqueKey))
                                {
                                    processedIdentifiers.insert(uniqueKey);
                                    tree->changeVisibilityState(treeItem->getObjectId(), true);
                                }
                            }
                        }
                    }
                }
            }
        }

        renderer->refreshForVisibilityAndSolidChanges();
        renderer->render();

        if (doc->getObjectTreeWidget())
        {
            doc->getObjectTreeWidget()->viewport()->update();
            doc->getObjectTreeWidget()->update();
        }
        if (doc->getViewport())
        {
            QWidget *viewportWidget = dynamic_cast<QWidget *>(doc->getViewport());
            if (viewportWidget)
            {
                viewportWidget->update();
            }
        }
        if (mainWindow->centralWidget())
        {
            mainWindow->centralWidget()->update();
        }
    }
}

void VVWidget::updateSummary(
    int errors,
    int warnings,
    int passed)
{
    summaryLabel->setText("Errors: " + QString::number(errors) + " | Warnings: " + QString::number(warnings) + " | Passed: " + QString::number(passed));
}
