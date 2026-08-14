#include "VVTestSelectionDialog.h"
#include "../core/VVBackendTests.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QSplitter>
#include <QListWidget>
#include <QStackedWidget>

VVTestSelectionDialog::VVTestSelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Select Tests To Run");
    resize(750, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QCheckBox *selectAllBox = new QCheckBox("Select All Suites", this);
    selectAllBox->setChecked(true);
    selectAllBox->setStyleSheet("font-weight: bold; padding-bottom: 5px;");
    mainLayout->addWidget(selectAllBox);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    QListWidget *suiteSideList = new QListWidget(splitter);
    QStackedWidget *containerStack = new QStackedWidget(splitter);

    QMap<QString, QMap<QString, QStringList>> layoutMap = VVBackendTests::getDoubleGroupedTestSuites();

    for (auto suiteIt = layoutMap.constBegin(); suiteIt != layoutMap.constEnd(); ++suiteIt)
    {
        suiteSideList->addItem(suiteIt.key());

        QScrollArea *scrollArea = new QScrollArea(containerStack);
        scrollArea->setWidgetResizable(true);
        QWidget *scrollContent = new QWidget(scrollArea);
        scrollContent->setObjectName("vvTestScrollContent");
        QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);

        QMap<QString, QStringList> groups = suiteIt.value();
        for (auto groupIt = groups.constBegin(); groupIt != groups.constEnd(); ++groupIt)
        {
            QGroupBox *groupXml = new QGroupBox(groupIt.key().toUpper() + " Constraints", scrollContent);
            QVBoxLayout *groupLayout = new QVBoxLayout(groupXml);

            for (const QString &testName : groupIt.value())
            {
                QCheckBox *box = new QCheckBox(testName, groupXml);
                box->setChecked(true);
                testBoxes.append(box);
                groupLayout->addWidget(box);
            }
            scrollLayout->addWidget(groupXml);
        }
        scrollLayout->addStretch();
        scrollContent->setLayout(scrollLayout);
        scrollArea->setWidget(scrollContent);
        containerStack->addWidget(scrollArea);
    }

    splitter->addWidget(suiteSideList);
    splitter->addWidget(containerStack);
    splitter->setSizes(QList<int>() << 200 << 550);
    mainLayout->addWidget(splitter);

    connect(suiteSideList, &QListWidget::currentRowChanged, containerStack, &QStackedWidget::setCurrentIndex);
    suiteSideList->setCurrentRow(0);

    connect(selectAllBox, &QCheckBox::toggled, this, [this](bool checked) {
        for (QCheckBox *box : testBoxes) {
            box->setChecked(checked);
        }
    });

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
}

QStringList VVTestSelectionDialog::getSelectedTests() const
{
    QStringList selected;
    for (QCheckBox *box : testBoxes)
    {
        if (box->isChecked()) selected.append(box->text());
    }
    return selected;
}
