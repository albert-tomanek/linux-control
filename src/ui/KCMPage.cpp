#include "KCMPage.h"

#include <QFileInfo>
#include <QtWidgets>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <KCModuleLoader>
#include <KQuickConfigModule>
#include <AeroQt/util/scopefn.h>

KCMPage::KCMPage(Aero::Browser *browser, KPluginMetaData kcmMeta, QWidget *parent) :
    PageBase(browser, parent)
{
    m_kcmName = QFileInfo(kcmMeta.fileName()).completeBaseName();

    KCModule *module = KCModuleLoader::loadModule(kcmMeta, this);

    if (!module) {
        qDebug() << ("Error loading " + m_kcmName);
        return;
    }

    QWidget *kcmWidget = module->widget();

    if (kcmWidget) {
        // 1. Force the embedded KCM widget to expand and fill available layout space
        kcmWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // 2. Fix QQuickWidget sizing if the KCM uses an underlying QQuickWidget
        if (auto *quickWidget = kcmWidget->findChild<QQuickWidget *>()) {
            quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }
    }

    QPushButton *applyBtn = nullptr;
    QDialogButtonBox *buttonBox = nullptr;

    const KCModule::Buttons buttons = module->buttons();

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    if (kcmWidget) {
        mainLayout->addWidget(kcmWidget, 1); // Stretch factor 1
    }

    buttonBox = new QDialogButtonBox(this) + also {
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
    };

    if (buttonBox && buttonBox->buttons().isEmpty()) {
        buttonBox->deleteLater();
    } else {
        mainLayout->addWidget(buttonBox, 0);
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
