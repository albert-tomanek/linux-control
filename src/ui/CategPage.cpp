#include "CategPage.h"

#include <QFileInfo>
#include <KConfigGroup>
#include <KDesktopFile>
#include <QDirIterator>
#include <QMap>
#include <QSet>
#include <QStandardPaths>
#include <QContextMenuEvent>
#include <QtWidgets>

#include <AeroQt/page.h>
#include <AeroQt/actionpgph.h>
#include <AeroQt/util/scopefn.h>

#include "KCMPage.h"

#include "settingscfg.h"

struct CategoryInfo {
    QString categ;  // X-KDE-System-Settings-Category
    QString parent; // X-KDE-System-Settings-Parent-Category
    QString name;   // Localized Name, resolved via KDesktopFile
    QString icon;   // Icon
};

static QMap<QString, CategoryInfo> loadKcmCategoryInfo()
{
    QMap<QString, CategoryInfo> result; // key: categ id
    QSet<QString> seenIds;

    const QStringList dirs = QStandardPaths::locateAll(
        QStandardPaths::GenericDataLocation,
        QStringLiteral("systemsettings/categories"),
        QStandardPaths::LocateDirectory);

    for (const QString &dir : dirs) {
        QDirIterator it(dir, {QStringLiteral("*.desktop")}, QDir::Files);
        while (it.hasNext()) {
            const QString path = it.next();
            KDesktopFile file(path);
            const KConfigGroup g = file.desktopGroup();

            const QString categ = g.readEntry("X-KDE-System-Settings-Category");
            if (categ.isEmpty() || seenIds.contains(categ))
                continue;
            seenIds.insert(categ);

            result.insert(categ, {categ,
                                  g.readEntry("X-KDE-System-Settings-Parent-Category"),
                                  file.readName(),
                                  file.readIcon()});
        }
    }

    return result;
}

QMap<QString, QString> transplants = {
    {"/system-administration/kcm_updates", "/software/kcm_updates"}
};

QAction *CategPage::registerRoot(Aero::Browser *br, QList<KPluginMetaData> allKcms)
{
    auto categInfo = loadKcmCategoryInfo();

    for (auto it = categInfo.begin(); it != categInfo.end(); ++it) {
        auto categId = it.key();
        auto categ = it.value();
        QString categPath = "/";

        for (QString categId = categ.categ; !categId.isEmpty(); categId = categInfo[categId].parent)
            categPath.prepend("/" + categId);

        if (categId == "rootcategory")
            continue;

        for (auto kcm: allKcms) {
            QString kcmCateg = kcm.value("X-KDE-System-Settings-Parent-Category");

            if (kcmCateg == categId) {
                auto kcmPath = categPath + kcm.pluginId();

                if (transplants.contains(kcmPath))
                    kcmPath = transplants[kcmPath];

                if (!br->allPaths().contains(kcmPath))
                    br->addPage(kcmPath, [=](auto args) { return new KCMPage(br, kcm); }) + also {
                        it->setText(kcm.name().isEmpty() ? kcm.pluginId() : kcm.name());
                        it->setToolTip(kcm.description());
                        it->setIcon(QIcon::fromTheme(kcm.iconName()));
                        it->setCheckable(true);     // This lets us have it checkable in the sidebar
                    };
            }
        }

        br->addPage(categPath, [=](auto args) { return new CategPage(br, categPath); }) + also {
            it->setText(categ.name);
            it->setIcon(QIcon::fromTheme(categ.icon));
        };
    }

    auto addCustomCateg = [&](QString categPath) {
        return br->addPage(categPath, [=](auto args) { return new CategPage(br, categPath); });
    };

    addCustomCateg("/software") + also {
        it->setText("Software");
        it->setToolTip("Manage the software on this system");
        it->setIcon(QIcon::fromTheme("system-software-install"));
    };

    addCustomCateg("/other") + also {
        it->setText("Other");
        it->setIcon(QIcon::fromTheme("systemsettings"));
    };

    return br->addPage("/", [=](auto args) { return new CategPage(br, "", true); }) + also {
        it->setText("Control Panel Home");
        it->setIcon(QIcon::fromTheme("systemsettings"));
    };
}

CategPage::CategPage(Aero::Browser *br, QString pathParent, bool expandCategs, QWidget *parent)
    : PageBase(br, parent)
{
    if (SettingsCfg::self()->iconView()) {
        QListWidget *lw;

        setLayout(new QVBoxLayout + also {
            it->setContentsMargins(0,0,0,0);
            it->addWidget(lw = new QListWidget + also {
                it->setViewMode(QListView::IconMode);
                it->setFrameShape(QFrame::NoFrame);
                it->setWordWrap(true);
                it->setWrapping(true);  // item wrap
                it->setResizeMode(QListView::Adjust);
            });
        });

        {
            // Item size
            int icoSize = style()->pixelMetric(QStyle::PM_LargeIconSize);
            lw->setGridSize(QSize(icoSize * 2, icoSize * 1.8 + 16));

            // Padding
            // style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + also {
            //     lw->viewport()->setContentsMargins(it, it, it, it);
            // };
            lw->setSpacing(12);
        }

        connect(lw, &QListWidget::itemActivated, [=](QListWidgetItem *item) {
            item->data(Qt::UserRole).value<QAction *>()->trigger();
        });

        // Populate

        for (QAction *act: br->children(pathParent)) {
            lw->addItem(new QListWidgetItem + also {
                it->setText(act->text());
                it->setIcon(act->icon());
                it->setToolTip(act->toolTip() == act->text().replace("&", "") ? QString() : act->toolTip());
                it->setData(Qt::UserRole, QVariant::fromValue(act));
            });
        }
    }
    else if (!expandCategs) {
        Aero::ActionPgph *pgph;

        setLayout(new QVBoxLayout + also {
            it->setContentsMargins(0,0,0,0);
            it->addWidget(new Aero::Page() + also {
                it->layout()->addWidget(pgph = new Aero::ActionPgph("", QIcon(), true) + also {
                    it->setTitleAction(br->actionForPath(pathParent));
                    it->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
                });
            });
        });

        for (QAction *child: br->children(pathParent))
            pgph->addAction(child);
    }
    else {
        QAction *categAct = br->actionForPath(pathParent);

        setLayout(new QVBoxLayout + also {
            it->setContentsMargins(0,0,0,0);
            it->addWidget(new Aero::Page(categAct->text()) + also {
                it->layout()->addLayout(m_flow = new FlowLayout(nullptr, 12, 12, 12));
            });
        });

        for (QAction *categAct: br->children(pathParent)) {
            auto *pgph = new Aero::ActionPgph() + also {
                it->setTitleAction(categAct);
                it->setFixedWidth(300);
            };

            m_flow->addItem(new QVBoxLayout + also {
                it->addWidget(pgph);
                it->addStretch(1);
            });

            for (QAction *childAct: br->children(categAct))
                pgph->addAction(childAct);
        }
    }

    addContextMenu(this);
}

void CategPage::addContextMenu(QWidget *targetWidget)
{
    // targetWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    // connect(targetWidget, &QWidget::customContextMenuRequested, [=](const QPoint &pos) {

    overrideEvent<QContextMenuEvent>(targetWidget, QEvent::ContextMenu, [=](QContextMenuEvent *menuEvent) {
        QMenu menu;

        QAction *action1 = menu.addAction(QIcon::fromTheme("view-list-icons"), "Icon view") + also {
            it->setCheckable(true);
            bind_prop_bidi(
                SettingsCfg::self(), "iconView", &SettingsCfg::iconViewChanged,
                it, "checked", &QAction::toggled,
                true
            );
        };
        // menu.addSeparator();

        QAction *selectedAction = menu.exec(
            menuEvent->globalPos()
        );
    });
}
