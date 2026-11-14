#ifndef PLUGIN_INCLUDED
#define PLUGIN_INCLUDED

#include <string>

#include <QString>
#include <QWidget>

#include <brlcad/Database/Database.h>


class Plugin {
public:
    virtual ~Plugin(void) {}

    class AppData {
    public:
        virtual ~AppData(void) {}

        virtual QWidget*          ParentWidget(void) = 0;
        virtual BRLCAD::Database& Db(void)           = 0;

    protected:
        AppData(void) {}
        AppData(const AppData&) {}
        const AppData& operator=(const AppData&) {return *this;}
    };

    virtual QString Name(void)            = 0;
    virtual QString Description(void)     = 0;

    virtual void    Handle(AppData& data) = 0;

protected:
    Plugin(void) {}
    Plugin(const Plugin&) {}
    const Plugin& operator=(const Plugin&) {return *this;}
};
#endif // PLUGIN_INCLUDED
