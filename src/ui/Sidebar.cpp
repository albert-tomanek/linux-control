#include <AeroQt/util/objecteventlistener.h>
#include <AeroQt/util/props.h>
#include <AeroQt/util/scopefn.h>
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

    auto *outerV = new QVBoxLayout;
    outerV->setContentsMargins(0, 0, 0, 0);
    outerV->setSpacing(0);

    this->setWidget(new QFrame + also {
        it->setObjectName("navPane");
        it->setLayout(outerV);
    });

    // Text lives in a child widget so a fade effect touches only the text,
    // never the pane background.
    outerV->addWidget(
        this->textWrap = new QWidget + also {
            it->setStyleSheet("background: transparent;");
            it->setLayout(this->navV = new QVBoxLayout + also {
                it->setContentsMargins(12, 14, 8, 10);
                it->setSpacing(0);
            });
        }
    );

    /* Effects */
    {
        m_sidebarTextEffect = new QGraphicsOpacityEffect(textWrap);
        textWrap->setGraphicsEffect(m_sidebarTextEffect);
        m_sidebarTextEffect->setOpacity(1.0);
    }

    navV->addLayout(m_itemsL = new QFormLayout + also {
        it->setSpacing(6);
        it->setHorizontalSpacing(0);
    });

    navV->addStretch(1);

    navV->addWidget(m_seeAlsoLabel = new QLabel("See also") + also {
        QFont f = it->font();
        f.setPointSize(8);
        it->setFont(f);
        it->setStyleSheet("color: #666666; background: transparent;");
        it->hide();
    });

    navV->addLayout(m_seeAlsoL = new QFormLayout + also {
        it->setContentsMargins(0, 8, 0, 0);
        it->setSpacing(6);
    });

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
    auto *fadeAnim = new QPropertyAnimation(fadeEffect, "opacity", this) + also {
        it->setStartValue(0.0);
        it->setEndValue(1.0);
        it->setDuration(2000);
        it->setEasingCurve(QEasingCurve::OutCubic);
    };

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
            (new QPropertyAnimation(m_sidebarTextEffect, "opacity", m_sidebarTextEffect) + also {
                it->setStartValue(1.0);
                it->setEndValue(0.0);
                it->setDuration(300);
                it->setEasingCurve(QEasingCurve::InCubic);
                QObject::connect(it, &QPropertyAnimation::finished, act, &QAction::trigger);
            })->start(QAbstractAnimation::DeleteWhenStopped);
        }
        else {
            QMetaObject::invokeMethod(act, &QAction::trigger, Qt::QueuedConnection);
        }
    };

    indicator = nullptr;

    /* Create the actual widget */

    if (act->isCheckable()) {
        /* Act is part of a group of destinations that switch */

        auto *but = new QLabel + also {
            it->setText(act->text());
            it->setToolTip(act->toolTip());
            it->setWhatsThis(act->whatsThis());

            it->setWordWrap(true);
        };

        underlineOnHover(but, [=](){ return act->isEnabled(); });

        onEvent(but, QEvent::MouseButtonRelease, [=](QEvent *evt) {
            triggerAction();
        });

        indicator = new QLabel;
        if (auto *ag = act->actionGroup())
            if (ag->isExclusive())
                bind_prop(ag, "enabled", indicator, "text", &QActionGroup::triggered, true, [=](auto _) {   // We're not actually binding to "enabled", we're just using this func for brevity and to call syncOnCreate
                    bool isSelected = ag->checkedAction() == act;

                    but->setFont(but->font() + also {
                        it.setBold(isSelected);
                    });

                    return QVariant(isSelected ? "<strong>\u25cf</strong>" : "");
                });

        widget = but;
    }
    else if (auto *wa = qobject_cast<QWidgetAction *>(act)) {
        widget = wa->defaultWidget();
    }
    else {
        auto *but = new QLabel + also {
            it->setText(act->text());
            it->setToolTip(act->toolTip());
            it->setWhatsThis(act->whatsThis());

            it->setWordWrap(true);
        };

        underlineOnHover(but, [=](){ return act->isEnabled(); });

        onEvent(but, QEvent::MouseButtonRelease, [=](QEvent *evt) {
            triggerAction();
        });

        widget = but;
    }

    /* Fill the indicator if it hasn't been filled yet */
    if (!indicator) {
        if (QVariant v = act->property("elevatedPriv"); v.isValid() && v.value<bool>()) {
            indicator = new QLabel + also {
                it->setPixmap(
                    QIcon::fromTheme("gtk-dialog-authentication")
                        .pixmap(QSize(16, 16))
                );
            };
        }
        else if (m_showIcons && !act->icon().isNull()) {
            indicator = new QLabel + also {
                it->setPixmap(
                    act->icon().pixmap(QSize(16, 16))
                );
            };
        }
    }
}


static void underlineOnHover(QWidget *but, std::function<bool()> shouldUnderline)
{
    onEvent(but, QEvent::Enter, [=](QEvent *) {
        but->setFont(but->font() + also {
            it.setUnderline(shouldUnderline());
        });
    });

    onEvent(but, QEvent::Leave, [=](QEvent *) {
        but->setFont(but->font() + also {
            it.setUnderline(false);
        });
    });
}
