#include "HomePage.h"

#include <AeroQt/actionpgph.h>
#include <AeroQt/util/scopefn.h>

HomePage::HomePage(Aero::Browser *br, QWidget *parent)
    : PageBase(br, parent)
{
    m_flow = new FlowLayout(this, 12, 12, 12);

    makeCategs();

    for (auto pgPath: m_browser->allPaths()) {
        bool sorted = false;

        for (auto categ: m_categs.keys())
            if (pgPath.startsWith("/"+categ+"/")) {
                m_categs[categ]->addAction(
                    m_browser->actionForPath(pgPath)
                );
                sorted = true;
                break;
            }

        if (!sorted)
            m_categOther->addAction(
                m_browser->actionForPath(pgPath)
            );
    }
}

void HomePage::makeCategs()
{
    // The ppssible IDs come directly from "X-KDE-System-Settings-Parent-Category"

    auto makeCateg = [&](QString id, QString title = QString()) {
        m_categs[id] = new Aero::ActionPgph(title.isEmpty() ? id : title) + also {
            it->setFixedWidth(240);
            m_flow->addWidget(it);
        };
    };

    makeCateg("appearance", "Appearance");
    makeCateg("system-administration", "System administration");
    makeCateg("security-privacy", "Security & Privacy");
    makeCateg("hardware", "Hardware");
    makeCateg("session");
    makeCateg("windowmanagement");
    makeCateg("themes");
    makeCateg("networksettings", "Network settings");
    makeCateg("keyboard");
    makeCateg("applications");
    makeCateg("workspace");
    makeCateg("pointing-devices");
    makeCateg("aeroshell");
    makeCateg("rootcategory");
    makeCateg("display");
    makeCateg("input-devices");
    makeCateg("applications-permissions");
    makeCateg("hardware-input-touchscreen");
    makeCateg("search");
    makeCateg("regionalsettings");
    makeCateg("font");
    makeCateg("applications-defaults", "Default applications");
    makeCateg("removable-storage");

    m_categOther = new Aero::ActionPgph("Other") + also {
        it->setFixedWidth(240);
        m_flow->addWidget(it);
    };
}
