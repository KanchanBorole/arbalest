#ifndef VVCONTROLLER_H
#define VVCONTROLLER_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include "VVPlugin.h"

class QMenuBar;
class QWidget;
class VVWidget;

class VVController : public QObject
{
    Q_OBJECT
public:
    explicit VVController(const VVPlugin::Host& host);

    QWidget*        CreatePanel(void);
    void            CreateMenu(QMenuBar* menuBar);
    QList<QWidget*> CreateToolbarButtons(QWidget* parent);
    void            Reset(void);

private:
    VVPlugin::Host             m_host;
    QWidget*                   m_parentWidget;

    VVWidget*                  vvWidget;
    QTimer*                    vvTimer;
    QStringList                vvValidationQueue;
    bool                       vvRunning;
    int                        vvCurrentIndex;
    QStringList                vvTests;
    QMap<QString, QStringList> vvSuites;

private slots:
    void startVVValidation();
    void processVVValidation();
    void stopVVValidation();
};
#endif // VVCONTROLLER_H
