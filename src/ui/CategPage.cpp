#include "CategPage.h"

#include <QFileInfo>

#include <AeroQt/page.h>
#include <AeroQt/actionpgph.h>
#include <AeroQt/util/scopefn.h>

#include "KCMPage.h"

QAction *CategPage::registerRoot(Aero::Browser *br, QList<KPluginMetaData> allKcms)
{
    QMap<QString, QAction *> categPages;

    auto makeCateg = [=, &categPages](QString id, QString iconName = QString(), QString title = QString(), QList<QString> nestedCatIds = {}) mutable {
        auto thisCategPath = QString("/%1").arg(id);
        QList<QAction *> thisCategPages;

        for (auto nestedCatId: nestedCatIds)
            thisCategPages += categPages[nestedCatId];

        for (auto kcm: allKcms) {
            QString kcmCateg = kcm.value("X-KDE-System-Settings-Parent-Category");
            QString kcmName = QFileInfo(kcm.fileName()).completeBaseName();

            if (kcmCateg == id) {
                thisCategPages += br->addPage(thisCategPath + "/" + kcmName, [=](auto args) { return new KCMPage(br, kcm); }) + also {
                    it->setText(kcm.name().isEmpty() ? kcmName : kcm.name());
                    it->setToolTip(kcm.description());
                    it->setIcon(QIcon::fromTheme(kcm.iconName()));
                    it->setCheckable(true);     // This lets us have it checkable in the sidebar
                };
            }
        }

        categPages[id] = br->addPage(thisCategPath, [=](auto args) { return new CategPage(br, thisCategPath); }) + also {
            it->setText(title.isEmpty() ? id + also { it[0] = it[0].toUpper(); } : title);
            it->setIcon(QIcon::fromTheme(iconName));
            it->setCheckable(true);     // This lets us have it checkable in the sidebar
        };
    };

    makeCateg("appearance", "", QString(), {"font", "themes"});
    makeCateg("system-administration", "", "Operating system");
    makeCateg("security-privacy", "preferences-security", "Security & Privacy");
    makeCateg("hardware", "", "Connected devices", {"removable-storage"});
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
    makeCateg("removable-storage", "", "Disks & cameras");
    makeCateg("other");

    return br->addPage("/", [=](auto args) { return new CategPage(br, "", true); }) + also {
        it->setText("Control Panel Home");
        it->setIcon(QIcon::fromTheme("systemsettings"));
    };
}

CategPage::CategPage(Aero::Browser *br, QString pathParent, bool expandCategs, QWidget *parent)
    : PageBase(br, parent)
{
    setLayout(new QVBoxLayout + also {
        it->setContentsMargins(0,0,0,0);
        it->addWidget(new Aero::Page() + also {
            it->layout()->addLayout(m_flow = new FlowLayout(nullptr, 12, 12, 12));
        });
    });

    auto *pgph = new Aero::ActionPgph() + also {
        it->setTitleAction(br->actionForPath(pathParent));
        it->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        m_flow->addWidget(it);
    };

    for (auto path: br->allPaths()) {
        if (path == pathParent) continue;

        if (path.left(path.lastIndexOf("/")) == pathParent)
            pgph->addAction(
                br->actionForPath(path)
            );
    }
}
