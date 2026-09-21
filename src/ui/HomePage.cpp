#include "HomePage.h"

#include <AeroQt/page.h>
#include <AeroQt/actionpgph.h>
#include <AeroQt/util/scopefn.h>

HomePage::HomePage(Aero::Browser *br, QWidget *parent)
    : PageBase(br, parent)
{
    setLayout(new QVBoxLayout + also {
        it->setContentsMargins(0,0,0,0);
        it->addWidget(new Aero::Page("Adjust your computer's settings") + also {
            it->layout()->addLayout(m_flow = new FlowLayout(nullptr, 12, 12, 12));
        });
    });

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
    }
}

void HomePage::makeCategs()
{
    auto makeCateg = [&](QString id, QString iconName = QString(), QString title = QString()) {
        auto categPath = QString("/%1").arg(id);
        Aero::ActionPgph *pgph = new Aero::ActionPgph();
        pgph->setFixedWidth(240);

        if (auto *sectionAction = m_browser->actionForPath(categPath))
            pgph->setTitleAction(sectionAction);
        else {
            pgph->setTitleAction(m_browser->addPage(categPath, nullptr) + also {
                if (title.isEmpty())
                    it->setText(id + also { it[0] = it[0].toUpper(); });
                else
                    it->setText(title);

                it->setIcon(QIcon::fromTheme(iconName));
            });
        }

        m_flow->addItem(new QVBoxLayout + also {
            it->addWidget(pgph);
            it->addStretch(1);
        });

        // m_flow->addWidget(pgph);
        m_categs[id] = pgph;
    };

    // The possible IDs come directly from "X-KDE-System-Settings-Parent-Category"

    makeCateg("appearance");
    makeCateg("system-administration", "", "Operating system");
    makeCateg("security-privacy", "preferences-security", "Security & Privacy");
    makeCateg("hardware");
    makeCateg("session");
    makeCateg("windowmanagement", "", "Window management");
    makeCateg("themes", "", "Look & Feel");
    makeCateg("networksettings", "preferences-system-network", "Network settings");
    makeCateg("keyboard");
    makeCateg("applications");
    makeCateg("workspace");
    makeCateg("pointing-devices", "", "Pointing devices");
    makeCateg("rootcategory");
    makeCateg("display");
    makeCateg("input-devices", "", "Input devices");
    makeCateg("applications-permissions", "", "Permissions");
    makeCateg("hardware-input-touchscreen", "", "Touchscreen");
    makeCateg("search");
    makeCateg("regionalsettings", "preferences-desktop-locale", "Region & Language");
    makeCateg("font");
    makeCateg("applications-defaults", "", "Default applications");
    makeCateg("removable-storage", "", "Removable storage");
    makeCateg("other");
}
