#include "VVDockWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>

VVDockWidget::VVDockWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel("Verification & Validation");
    layout->addWidget(title);

    QPushButton *runButton = new QPushButton("Run V&V Test");
    layout->addWidget(runButton);

    output = new QTextEdit();
    output->setReadOnly(true);
    layout->addWidget(output);

    connect(runButton, &QPushButton::clicked, this, [this]()
            { output->append("V&V plugin loaded successfully."); });
    setLayout(layout);
}
