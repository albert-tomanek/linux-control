#include "ui_NewMainWindow.h"

#include <QtWidgets>

#include <KPluginMetaData>
#include <KCModuleLoader>
#include <KCModuleData>

#include <AeroQt/insetwindow.h>
#include <AeroQt/page.h>
#include <AeroQt/actionpgph.h>
#include <AeroQt/util/scopefn.h>
#include <AeroQt/util/deepbind.h>

#include "NewMainWindow.h"

NewMainWindow::NewMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NewMainWindow)
{
    ui->setupUi(this);

    m_br = new Aero::Browser("/", this);

    // Header

    ui->header->layout()->addWidget(m_br->navButtons());
    ui->header->layout()->addWidget(m_br->addressBar());
    ui->scrollArea->setWidget(m_br->pageFrame() + also {
        it->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    });

    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
    ui->splitter->setSizes(QList<int>{1, 10000});

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
    m_br->addPage("/", [=](auto _) { return this->makeHomePage(); }) + also {
        it->setText("Control Center");
        it->setIcon(QIcon::fromTheme("systemsettings"));
    };

    populateKcms();

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
}

QWidget* createModuleContainer(KCModule *module, QWidget *parent = nullptr)
{
    if (!module) {
        return nullptr;
    }

    QPushButton *applyBtn = nullptr;
    QDialogButtonBox *buttonBox = nullptr;

    const KCModule::Buttons buttons = module->buttons();

    auto *container = new QWidget(parent) + also {
        it->setLayout(new QVBoxLayout + also {
            it->setContentsMargins(0, 0, 0, 0);
            it->addWidget(module->widget(), 1);

            it->addWidget(buttonBox = new QDialogButtonBox + also {
                if (buttons.testFlag(KCModule::Apply)) {
                    applyBtn = it->addButton(QDialogButtonBox::Apply);
                    applyBtn->setEnabled(false);
                    QObject::connect(applyBtn, &QPushButton::clicked, module, &KCModule::save);
                }

                if (buttons.testFlag(KCModule::Default)) {
                    auto *defBtn = it->addButton(QDialogButtonBox::RestoreDefaults);
                    QObject::connect(defBtn, &QPushButton::clicked, module, &KCModule::defaults);
                }

                if (buttons.testFlag(KCModule::Help)) {
                    auto *helpBtn = it->addButton(QDialogButtonBox::Help);
                    // QObject::connect(helpBtn, &QPushButton::clicked, module, ...);
                }
            });
        });
    };

    if (buttonBox && buttonBox->buttons().isEmpty()) {
        buttonBox->deleteLater();
    }

    QObject::connect(module, &KCModule::needsSaveChanged, container, [=]() {
        if (applyBtn) {
            applyBtn->setEnabled(module->needsSave());
        }
    });

    return container;
}

void NewMainWindow::populateKcms()
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

    // 3. Deduplicate plugins by pluginId
    QSet<QString> seenIds;

    for (const KPluginMetaData kcm: plugins) {
        const QString id = kcm.pluginId();
        if (id.isEmpty() || seenIds.contains(id))
            continue;

        seenIds.insert(id);

        auto makeWidget = [=](QMap<QString, QString> args) -> QWidget* {
            KCModule *kcmModule = KCModuleLoader::loadModule(kcm);

            return createModuleContainer(kcmModule);
        };

        m_kcmActions += m_br->addPage("/kcm/" + id, makeWidget) + also {
            it->setText(kcm.name().isEmpty() ? id : kcm.name());
            it->setToolTip(kcm.description());
            it->setIcon(QIcon::fromTheme(kcm.iconName()));
        };
    }
}

QWidget *NewMainWindow::makeHomePage()
{
    auto *p = new Aero::Page("Settings", nullptr);

    p->layout()->addWidget(new Aero::ActionPgph("KCM Settings Pages") + also {
        for (auto *act: m_kcmActions) {
            it->addAction(act);
            qDebug()<<act;
        }
        it->setIcon(QIcon::fromTheme("systemsettings"));
    });

    return p;
}
