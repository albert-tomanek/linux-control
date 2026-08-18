#include <AeroQt/util/objecteventlistener.h>
#include <AeroQt/util/props.h>
#include <AeroQt/branding.h>

#include <functional>

#include "Sidebar.h"

using namespace Aero;

// https://www.eduroam.cz/_media/en/uzivatel/sw/win/seven04en.png
// TODO: avatar in UserAccountsPage.cpp:177
// convert SidebarLink to QAction somehow

static void underlineOnHover(QWidget *but, std::function<bool()> shouldUnderline);

Sidebar::Sidebar(QAction *goHome, int initialWidth, QWidget *parent, bool showIcons) :
    QScrollArea(parent),
    m_goHome(goHome),
    m_showIcons(showIcons),
    m_initialWidth(initialWidth),
    m_fadeInText(false),
    m_fadeOutText(false)
{
    /* This */
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setFrameShape(QFrame::NoFrame);
    this->setWidgetResizable(true);

    /* Palette */

    // Won't work while QSS is applied at root by libAeroQt

    // QPalette pal = viewport()->palette();
    // pal.setColor(QPalette::Base, QColor("#F1F4F9"));
    // viewport()->setPalette(pal);
    // QPalette pal = palette();
    // pal.setColor(QPalette::WindowText, QColor("#151c55"));
    // setPalette(pal);

    if (Aero::Branding::reportedBuild() >= 7600)
        setStyleSheet("color: #151c55;");
    else
        setStyleSheet("color: white;");

    /* Children */

    auto *pane = new QFrame;
    pane->setObjectName("navPane");
    auto *outerV = new QVBoxLayout(pane);
    outerV->setContentsMargins(0, 0, 0, 0);
    outerV->setSpacing(0);

    this->setWidget(pane);

    // Text lives in a child widget so a fade effect touches only the text,
    // never the pane background.
    this->textWrap = new QWidget;
    textWrap->setStyleSheet("background: transparent;");
    this->navV = new QVBoxLayout(textWrap);
    navV->setContentsMargins(12, 14, 8, 10);
    navV->setSpacing(0);
    outerV->addWidget(textWrap);

    /* Effects */
    {
        m_sidebarTextEffect = new QGraphicsOpacityEffect(textWrap);
        textWrap->setGraphicsEffect(m_sidebarTextEffect);
        m_sidebarTextEffect->setOpacity(1.0);
    }

    m_itemsL = new QFormLayout;
    m_itemsL->setSpacing(6);
    m_itemsL->setHorizontalSpacing(0);
    navV->addLayout(m_itemsL);

    navV->addStretch(1);

    m_seeAlsoLabel = new QLabel("See also");
    QFont f = m_seeAlsoLabel->font();
    f.setPointSize(8);
    m_seeAlsoLabel->setFont(f);
    m_seeAlsoLabel->setStyleSheet("color: #666666; background: transparent;");
    m_seeAlsoLabel->hide();
    navV->addWidget(m_seeAlsoLabel);

    m_seeAlsoL = new QFormLayout;
    m_seeAlsoL->setContentsMargins(0, 8, 0, 0);
    m_seeAlsoL->setSpacing(6);
    m_itemsL->setHorizontalSpacing(0);
    navV->addLayout(m_seeAlsoL);

    if (m_goHome)
    {
        QWidget *w, *indic;
        widgetForAction(m_goHome, w, indic);
        navV->insertSpacing(0, 16);
        navV->insertWidget(0, w);
    }
}

void Sidebar::setFadeInText(bool b)
{
    m_fadeInText = b;

    auto *fadeEffect = new QGraphicsOpacityEffect(textWrap);
    textWrap->setGraphicsEffect(fadeEffect);
    fadeEffect->setOpacity(0.0);
    auto *fadeAnim = new QPropertyAnimation(fadeEffect, "opacity", this);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    fadeAnim->setDuration(2000);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);

    QTimer::singleShot(0, this, [fadeAnim]() { fadeAnim->start(); });
}

void Sidebar::setFadeOutText(bool b)
{
    m_fadeOutText = b;
}


void Sidebar::addItem(QAction *act)
{
    QWidget *w = nullptr;
    QWidget *icon = nullptr;

    widgetForAction(act, w, icon);

    if (icon) {
        m_itemsL->addRow(icon, w);

        m_itemsL->setHorizontalSpacing(6);    // Don't actually have any spacing until at least one item with a check indicator is added
    }
    else
        m_itemsL->addRow(nullptr, w);
}

void Sidebar::addSeeAlso(QAction *act)
{
    m_seeAlsoLabel->show();

    QWidget *w = nullptr;
    QWidget *icon = nullptr;

    widgetForAction(act, w, icon);

    if (icon) {
        m_seeAlsoL->addRow(icon, w);

        m_seeAlsoL->setHorizontalSpacing(6);    // Don't actually have any spacing until at least one item with a check indicator is added
    }
    else
        m_seeAlsoL->addRow(nullptr, w);
}

QSize Sidebar::sizeHint() const
{
    return QSize(m_initialWidth, QScrollArea::sizeHint().height());
}

void Sidebar::widgetForAction(QAction *act, QWidget *&widget, QWidget *&indicator)
{
    auto triggerAction = [=](){
        if (m_fadeOutText) {
            auto *anim = new QPropertyAnimation(m_sidebarTextEffect, "opacity", m_sidebarTextEffect);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            anim->setDuration(300);
            anim->setEasingCurve(QEasingCurve::InCubic);
            QObject::connect(anim, &QPropertyAnimation::finished, act, &QAction::trigger);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        else {
            QMetaObject::invokeMethod(act, &QAction::trigger, Qt::QueuedConnection);
        }
    };

    indicator = nullptr;

    /* Create the actual widget */

    if (act->isCheckable()) {
        /* Act is part of a group of destinations that switch */

        auto *but = new QLabel;

        but->setText(act->text());
        but->setToolTip(act->toolTip());
        but->setWhatsThis(act->whatsThis());

        underlineOnHover(but, [=](){ return act->isEnabled(); });
        but->setWordWrap(true);

        onEvent(but, QEvent::MouseButtonRelease, [=](QEvent *evt) {
            triggerAction();
        });

        indicator = new QLabel;
        if (auto *ag = act->actionGroup())
            if (ag->isExclusive())
                bind_prop(ag, "enabled", indicator, "text", &QActionGroup::triggered, true, [=](auto _) {   // We're not actually binding to "enabled", we're just using this func for brevity and to call syncOnCreate
                    bool isSelected = ag->checkedAction() == act;

                    QFont font = but->font();
                    font.setBold(isSelected);
                    but->setFont(font);

                    return QVariant(isSelected ? "<strong>\u25cf</strong>" : "");
                });

        widget = but;
    }
    else if (auto *wa = qobject_cast<QWidgetAction *>(act)) {
        widget = wa->defaultWidget();
    }
    else {
        auto *but = new QLabel;

        but->setText(act->text());
        but->setToolTip(act->toolTip());
        but->setWhatsThis(act->whatsThis());

        but->setWordWrap(true);
        underlineOnHover(but, [=](){ return act->isEnabled(); });

        onEvent(but, QEvent::MouseButtonRelease, [=](QEvent *evt) {
            triggerAction();
        });

        widget = but;
    }

    /* Fill the indicator if it hasn't been filled yet */
    if (!indicator) {
        if (QVariant v = act->property("elevatedPriv"); v.isValid() && v.value<bool>()) {
            auto *l = new QLabel;
            l->setPixmap(
                QIcon::fromTheme("gtk-dialog-authentication")
                    .pixmap(QSize(16, 16))
            );
            indicator = l;
        }
        else if (m_showIcons && !act->icon().isNull()) {
            auto *l = new QLabel;
            l->setPixmap(
                act->icon().pixmap(QSize(16, 16))
            );
            indicator = l;
        }
    }
}


static void underlineOnHover(QWidget *but, std::function<bool()> shouldUnderline)
{
    onEvent(but, QEvent::Enter, [=](QEvent *) {
        QFont f = but->font();
        f.setUnderline(shouldUnderline());
        but->setFont(f);
    });

    onEvent(but, QEvent::Leave, [=](QEvent *) {
        QFont f = but->font();
        f.setUnderline(false);
        but->setFont(f);
    });
}
