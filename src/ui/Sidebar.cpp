#include <AeroQt/util/objecteventlistener.h>
#include <AeroQt/util/props.h>

#include <functional>

#include "Sidebar.h"

static void underlineOnHover(QWidget *but, std::function<bool()> shouldUnderline);

QString kStyleSheet = (
    "QScrollArea { background: transparent; border: none; }"
    "QToolButton, QLabel"
        "{ border: none; background: transparent; color: #000000; text-align: left; padding: 0; font-size: 9pt; }"
    "QToolButton:hover, QLabel:hover"
        "{ color: #0033AA; text-decoration: underline; }"
    "QToolButton:hover:disabled, QLabel:hover:disabled"
        "{ text-decoration: none; color: gray; }"
);

Sidebar::Sidebar(int initialWidth, QWidget *parent) :
    QScrollArea(parent)
{
    this->setFixedWidth(initialWidth);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setFrameShape(QFrame::NoFrame);
    this->setWidgetResizable(true);
    this->setStyleSheet(kStyleSheet);

    auto *pane = new QFrame;
    pane->setObjectName("navPane");
    pane->setFixedWidth(168);
    pane->setStyleSheet(
        "#navPane { background: #F1F4F9; border-right: 1px solid #DCE0E8; }"
        );
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

    m_seeAlsoV = new QVBoxLayout;
    m_seeAlsoV->setContentsMargins(0, 8, 0, 0);
    m_seeAlsoV->setSpacing(6);
    navV->addLayout(m_seeAlsoV);

    m_goHome = new QAction("Control Panel Home", this);
    {
        QWidget *w, *indic;
        widgetForAction(goHome(), w, indic);
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
    QWidget *indic = nullptr;

    widgetForAction(act, w, indic);

    if (indic) {
        m_itemsL->addRow(indic, w);

        m_itemsL->setHorizontalSpacing(6);    // Don't actually have any spacing until at least one item with a check indicator is added
    }
    else
        m_itemsL->addRow(nullptr, w);
}

void Sidebar::addSeeAlso(QAction *act)
{
    QWidget *w, *indic;
    widgetForAction(act, w, indic);

    m_seeAlsoV->addWidget(w);
    m_seeAlsoLabel->show();
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
                    return QVariant(ag->checkedAction() == act ? "<strong>\u25cf</strong>" : "");
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
