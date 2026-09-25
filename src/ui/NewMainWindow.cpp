#include "ui_NewMainWindow.h"

#include <QtWidgets>
#include <QFileInfo>

#include <KPluginMetaData>
#include <KCModuleLoader>
#include <KCModuleData>
#include <KService>

#include <AeroQt/insetwindow.h>
#include <AeroQt/page.h>
#include <AeroQt/actionpgph.h>
#include <AeroQt/util/scopefn.h>
#include <AeroQt/util/deepbind.h>

#include "NewMainWindow.h"
#include "CategPage.h"
#include "KCMPage.h"

#include "all_pages.h"

NewMainWindow::NewMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NewMainWindow)
{
    ui->setupUi(this);

    m_br = new Aero::Browser("/", this);

    // Header

    qobject_cast<QBoxLayout *>(ui->header->layout())->insertWidget(0, m_br->navButtons());
    qobject_cast<QBoxLayout *>(ui->header->layout())->insertWidget(1, m_br->addressBar());
    ui->scrollArea->setWidget(m_br->pageFrame() + also {
        it->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    });

    // Sidebar

    ui->sb->setFadeInText(true);
    ui->sb->setFadeOutText(true);

    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
    ui->splitter->setSizes(QList<int>{1, 10000});

    QActionGroup *prevGrp = nullptr;

    connect(m_br, &Aero::Browser::pageChanged, [=]() mutable {
        qDebug()<<m_br->path();

        ui->sb->clear();

        ui->sb->setVisible(
            qobject_cast<CategPage *>(m_br->page()) == nullptr
        );

        // Add sidebar actions for pages within same folder

        if (ui->sb->isVisible()) {
            auto *grp = prevGrp = new QActionGroup(m_br->page()) + also {
                it->setExclusive(true);
            };

            for (QAction *sibling: m_br->children(m_br->path() + let { return it.left(it.lastIndexOf("/")); })) {
                grp->addAction(sibling);
                sibling->setChecked(sibling == m_br->actionForPath(m_br->path()));

                ui->sb->addAction(sibling);
            }
        }

        // Add suggested actions

        if (auto *pg = qobject_cast<PageBase *>(m_br->page())) {
            for (auto *act: pg->sidebarLinks())
                if (act)
                    ui->sb->addAction(act);

            for (auto *act: pg->sidebarSeeAlso())
                if (act)
                    ui->sb->addSeeAlso(act);
        }
    });

    //

    makePages();
    makeActions();

    Aero::makeInsetWindow(this, centralWidget(), ui->header);
}

NewMainWindow::~NewMainWindow()
{
    delete ui;
}

void NewMainWindow::makePages()
{
    /* Creation process:
     * QList<KCM> (where categ = "...") -> [Page("/<category>/<kcmName>")] -> HomePage([SubPage(searchPrefix="/<category>/")])
     */

#define ADD_PAGE(PageClassName)  \
    {     \
        m_nativePages += m_br->addPage(PageClassName::path, [=](auto args) -> QWidget* { return new PageClassName(m_br, nullptr); }) + also {   \
            it->setText(#PageClassName);  \
            it->setCheckable(true);  \
            PageClassName::initAction(it);  \
        };          \
    }

    FOREACH_PAGE_CLASS(ADD_PAGE);
#undef ADD_PAGE

    populateKcms();     // Makes homepage too

    // Pages that are actually actions:

    connect(m_br, &Aero::Browser::pageNotFound, [=](QString path, QMap<QString, QString> args) {
        // if (path == "/install" && args.contains("id")) {
        //     auto *wiz = new InstallWizard(m_inst) + also {
        //         it->setInstallId(args["id"]);
        //     };

        //     wiz->show();
        // }
    });
}

void NewMainWindow::makeActions()
{
    connect(ui->aQuit, &QAction::triggered, [=]() {
        qApp->quit();
    });

    // Shell commands

    auto runShell = [=](QStringList args) {
        if (!QProcess::startDetached(args.first(), args.mid(1)))
            QMessageBox::critical(this, "Error", QString("Error running the following command:\n\n") + args.join(" "));
    };

    connect(ui->aDevMgmt, &QAction::triggered, [=]() {
        runShell({"devmgmt"});
    });

    connect(ui->aGetWidgets, &QAction::triggered, [=]() {
        runShell({"knewstuff-dialog6", "/usr/share/knsrcfiles/plasmoids.knsrc"});
    });

    connect(ui->aWidgetExplorer, &QAction::triggered, [=]() {
        runShell({"qdbus6", "org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.toggleWidgetExplorer"});
    });
}

void NewMainWindow::populateKcms()  // Call this only _after_ native pages are added
{
    QList<KPluginMetaData> plugins;

    // 1. Discover all plasma/kcms paths dynamically from Qt search paths
    QStringList kcmSearchPaths;
    for (const QString &libPath : QCoreApplication::libraryPaths()) {
        QDir kcmDir(libPath + QStringLiteral("/plasma/kcms"));
        if (kcmDir.exists()) {
            kcmSearchPaths << kcmDir.absolutePath();
        }
    }

    // 2. Load top-level and subfolder plugins (systemsettings, kinfocenter, etc.)
    for (const QString &basePath : kcmSearchPaths) {
        plugins += KPluginMetaData::findPlugins(basePath);

        QDirIterator it(basePath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            plugins += KPluginMetaData::findPlugins(it.next());
        }
    }

    m_goHome = CategPage::registerRoot(m_br, plugins);
    ui->sb->setHomeAction(m_goHome);
    addAction(m_goHome);


    // 3. Deduplicate plugins by pluginId
    // QSet<QString> seenIds;

    // for (const KPluginMetaData kcm: plugins) {
    //     const QString id = kcm.pluginId();
    //     if (id.isEmpty() || seenIds.contains(id))
    //         continue;

    //     seenIds.insert(id);

    //     QString settingsCateg = kcm.value("X-KDE-System-Settings-Parent-Category");
    //     QString kcmName = QFileInfo(kcm.fileName()).completeBaseName();

    //     QString kcmPath = settingsCateg.isEmpty() ?
    //         QString("/other/%1").arg(kcmName) :
    //         QString("/%1/%2").arg(settingsCateg).arg(kcmName);

    //     if (!m_br->allPaths().contains(kcmPath))    // If the KCM we are inserting has been supplemented by a native page, don't add it.
    //         m_kcmPages += m_br->addPage(kcmPath, [=](auto args) { return new KCMPage(m_br, kcm); }) + also {
    //             it->setText(kcm.name().isEmpty() ? id : kcm.name());
    //             it->setToolTip(kcm.description());
    //             it->setIcon(QIcon::fromTheme(kcm.iconName()));
    //             it->setCheckable(true);     // This lets us have it checkable in the sidebar
    //         };
    // }

    qDebug()<<m_br->allPaths();
}
