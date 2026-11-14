#ifndef VVPLUGIN_INCLUDED
#define VVPLUGIN_INCLUDED

#include <functional>

#include <QList>
#include <QString>
#include <QStringList>

#include "Plugin.h"

class QMenuBar;
class QWidget;
class VVController;


class VVPlugin : public Plugin {
public:
    struct Host {
        QWidget*                               parent;           
        std::function<BRLCAD::Database*(void)> activeDatabase;   
        std::function<QStringList(void)>       topObjectPaths;   
        std::function<void(const QString&)>    selectObject;     
        std::function<void(void)>              showPanel;       
    };

    VVPlugin(void);
    ~VVPlugin(void) override;

    // the Plugin interface
    QString Name(void) override;
    QString Description(void) override;

    void    Handle(AppData& data) override;           

    
    void            SetHost(const Host& host);
    QWidget*        CreatePanel(void);                              
    void            CreateMenu(QMenuBar* menuBar);                  
    QList<QWidget*> CreateToolbarButtons(QWidget* parent);          
    void            Reset(void);                                    

private:
    Host          m_host;
    VVController* m_controller;
};
#endif // VVPLUGIN_INCLUDED
