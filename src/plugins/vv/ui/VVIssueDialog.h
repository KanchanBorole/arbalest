#ifndef VVISSUEDIALOG_H
#define VVISSUEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class VVIssueDialog : public QDialog
{
public:
    explicit VVIssueDialog(
        QString issueText,
        QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("V&V Issue Details");

        resize(400, 200);

        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *title = new QLabel("<b>Issue Details</b>");
        layout->addWidget(title);

        QLabel *details = new QLabel(issueText);
        details->setWordWrap(true);
        layout->addWidget(details);

        QString suggestion;

        if (issueText.contains("WARNING"))
        {
            suggestion =
                "Suggested Fix:\n"
                "- Review geometry naming\n"
                "- Remove temporary objects";
        }
        else if (issueText.contains("ERROR"))
        {
            suggestion =
                "Suggested Fix:\n"
                "- Correct geometry issue";
        }
        else
        {
            suggestion =
                "No action required.";
        }

        QLabel *suggestionLabel = new QLabel(suggestion);

        suggestionLabel->setWordWrap(true);

        layout->addWidget(suggestionLabel);

        QPushButton *closeButton = new QPushButton("Close");

        connect(closeButton,
                &QPushButton::clicked,
                this,
                &QDialog::accept);
        layout->addWidget(closeButton);
    }
};
#endif
