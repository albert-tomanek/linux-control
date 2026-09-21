#include "KCMPage.h"

#include <QFileInfo>
#include <QtWidgets>
#include <AeroQt/util/scopefn.h>

KCMPage::KCMPage(Aero::Browser *browser, KPluginMetaData kcmMeta, QWidget *parent) :
    PageBase(browser, parent)
{
    m_kcmName = QFileInfo(kcmMeta.fileName()).completeBaseName();

    KCModule *module = KCModuleLoader::loadModule(kcmMeta);

    if (!module) {
        qDebug()<<("Error loading " + m_kcmName);
        return;
    }

    QPushButton *applyBtn = nullptr;
    QDialogButtonBox *buttonBox = nullptr;

    const KCModule::Buttons buttons = module->buttons();

    this->setLayout(new QVBoxLayout + also {
        it->setContentsMargins(0, 0, 0, 0);
        it->addWidget(module->widget(), 1);

        it->addWidget(buttonBox = new QDialogButtonBox + also {
            it->setContentsMargins(4, 4, 4, 4);

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

    if (buttonBox && buttonBox->buttons().isEmpty()) {
        buttonBox->deleteLater();
    }

    QObject::connect(module, &KCModule::needsSaveChanged, this, [=]() {
        if (applyBtn) {
            applyBtn->setEnabled(module->needsSave());
        }
    });
}

QList<QAction *> KCMPage::sidebarLinks()
{
    return {};
}

QList<QAction *> KCMPage::sidebarSeeAlso()
{
    if (kcmName() == "kcm_networkmanagement")
        return {
            findPageForKcm(m_browser, "kcm_bluetooth"),
        };

    return {};
}
