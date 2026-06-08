#ifndef VVTESTSELECTIONDIALOG_H
#define VVTESTSELECTIONDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStringList>

class VVTestSelectionDialog
    : public QDialog
{
public:
    explicit VVTestSelectionDialog(
        QWidget *parent = nullptr);

    QStringList
    getSelectedTests() const;

private:
    QList<QCheckBox *> testBoxes;
};
#endif
