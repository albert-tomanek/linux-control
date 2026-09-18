#include "ui_NewMainWindow.h"

#include <QtWidgets>

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

    this->b = new Aero::Browser("/", this);

    // Header

    ui->header->layout()->addWidget(b->navButtons());
    ui->header->layout()->addWidget(b->addressBar());

    ui->splitter->replaceWidget(1, b->pageFrame() + also {
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
    b->Aero::Browser::addPage("/", nullptr) + also {
        it->setText("App Store");
        it->setIcon(QIcon::fromTheme("softwarecenter"));
    };
    // Pages that are actually actions:

    connect(b, &Aero::Browser::pageNotFound, [=](QString path, QMap<QString, QString> args) {
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
