#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QMap>
#include <QMenu>
#include <QClipboard>
#include <QApplication>

class VVWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VVWidget(QWidget *parent = nullptr);
    QTreeWidget *getIssueTree();

    void addIssue(const QString &issue);
    void addIssue(
        const QString &type,
        const QString &testName,
        const QString &description,
        const QString &objectName,
        const QString &fullPath);

    void clearIssues();

    void addIssue(
        const QString &type,
        const QString &issue);

    void appendConsoleMessage(
        const QString &message);

    void updateSummary(
        int errors, int warnings, int passed);

    void setValidationStatus(const QString &status);

signals:
    void geometrySelected(const QString &name);
    void issueDoubleClicked(QString issueText);

private:
    QVBoxLayout *layout;
    QLabel *titleLabel;
    QTreeWidget *issueTree;
    QPushButton *runButton;
    QPlainTextEdit *consoleOutput;
    QLabel *statusLabel;
    QLabel *summaryLabel;
    QMap<QString, QTreeWidgetItem *> objectGroups;

    void showContextMenu(const QPoint &pos);
};
