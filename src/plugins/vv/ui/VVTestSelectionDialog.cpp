#include "VVTestSelectionDialog.h"

VVTestSelectionDialog::VVTestSelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(
        "Select Tests To Run");
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QStringList tests = {
        "Duplicate ID Check",
        "No Empty Combos",
        "No Nested Regions",
        "Temporary Geometry Check",
        "Duplicate Geometry Names",
        "No Invalid Names",
        "Overlap Validation",
        "Volume Validation"};

    for (const QString &test : tests)
    {
        QCheckBox *box = new QCheckBox(test);
        box->setChecked(true);
        testBoxes.append(box);
        mainLayout->addWidget(box);
    }

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

    connect(okButton,
            &QPushButton::clicked,
            this,
            &QDialog::accept);

    connect(cancelButton,
            &QPushButton::clicked,
            this,
            &QDialog::reject);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);
}

QStringList
VVTestSelectionDialog::
    getSelectedTests() const
{
    QStringList selected;
    for (QCheckBox *box : testBoxes)
    {
        if (box->isChecked())
        {
            selected.append(
                box->text());
        }
    }
    return selected;
}
