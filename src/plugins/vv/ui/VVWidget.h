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
#include <QProgressBar> 

class QComboBox;
class MainWindow;

struct VVIssueRecord
{
    QString type;
    QString testName;
    QString description;
    QString objectName;
    QString fullPath;
    QString details;
    bool hasDetails = false;
    bool isSimple = false;
};

struct VVDocumentState
{
    QList<VVIssueRecord> issues;
    QString consoleText;
    QString statusText = "Validation idle";
    int errorCount = 0;
    int warningCount = 0;
    int passCount = 0;
    int progressValue = 0;
    bool progressVisible = false;
};

class VVWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VVWidget(QWidget *parent = nullptr);
    MainWindow *mainWindow;
    QTreeWidget *getIssueTree();

    void addIssue(
        const QString &type,
        const QString &testName,
        const QString &description,
        const QString &objectName,
        const QString &fullPath);

    void addIssueWithDetails(const QString &type,
                             const QString &testName,
                             const QString &description,
                             const QString &objectName,
                             const QString &fullPath,
                             const QString &rawDetails);

    void clearIssues();

    void addIssue(
        const QString &type,
        const QString &issue);

    VVDocumentState captureState() const;
    void restoreState(const VVDocumentState &state);
    void resetToIdle();

    void appendConsoleMessage(
        const QString &message);

    void updateSummary(
        int errors, int warnings, int passed);

    void setValidationStatus(const QString &status);

    void setProgress(int percentage);
    void hideProgress();
    void showProgress();
    QString getIconPath(const QString &type);

signals:
    void geometrySelected(const QString &name);
    void issueDoubleClicked(QString issueText);

public slots:
    void applyFilter(int index);

private:
    QVBoxLayout *layout;
    QLabel *titleLabel;
    QTreeWidget *issueTree;
    QPushButton *runButton;
    QPlainTextEdit *consoleOutput;
    QLabel *statusLabel;
    QLabel *summaryLabel;
    QMap<QString, QTreeWidgetItem *> objectGroups;
    QComboBox *filterDropdown;
    QProgressBar *progressBar;

    void showContextMenu(const QPoint &pos);
};
