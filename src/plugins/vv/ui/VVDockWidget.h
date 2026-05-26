#ifndef VVDOCKWIDGET_H
#define VVDOCKWIDGET_H
#include <QWidget>

class QTextEdit;
class VVDockWidget : public QWidget
{
public:
    explicit VVDockWidget(QWidget *parent = nullptr);

private:
    QTextEdit *output;
};
#endif