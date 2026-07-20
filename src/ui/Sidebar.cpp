#include <AeroQt/util/objecteventlistener.h>

#include "Sidebar.h"

void makeButtonWrapText(QAbstractButton *that);

QString kStyleSheet = (
    "QScrollArea { background: transparent; border: none; }"
    "QToolButton, QRadioButton"
        "{ border: none; background: transparent; color: #000000; text-align: left; padding: 0; font-size: 9pt; }"
    "QToolButton:hover, QRadioButton:hover"
        "{ color: #0033AA; text-decoration: underline; }"
    "QToolButton:hover:disabled, QRadioButton:hover:disabled"
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

    // Text lives in a child widget so a fade effect touches only the text,
    // never the pane background.
    this->textWrap = new QWidget;
    textWrap->setStyleSheet("background: transparent;");
    this->navV = new QVBoxLayout(textWrap);
    navV->setContentsMargins(12, 14, 8, 10);
    navV->setSpacing(0);
    outerV->addWidget(textWrap);

    m_goHome = new QAction("Control Panel Home", this);

    auto *controlHome = new QToolButton;
    controlHome->setDefaultAction(goHome());
    controlHome->setAutoRaise(true);
    controlHome->setCursor(Qt::PointingHandCursor);
    navV->addWidget(controlHome);
    navV->addSpacing(16);

    this->setWidget(pane);

    /* Effects */

    m_sidebarTextEffect = new QGraphicsOpacityEffect(textWrap);
    textWrap->setGraphicsEffect(m_sidebarTextEffect);
    m_sidebarTextEffect->setOpacity(1.0);
}

void Sidebar::setFadeInText(bool b)
{
    m_fadeOutText = b;

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


void Sidebar::addDest(QAction *act)
{
    auto triggerAction = [=](){
        if (m_fadeOutText) {
            auto *anim = new QPropertyAnimation(m_sidebarTextEffect, "opacity");
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            anim->setDuration(300);
            anim->setEasingCurve(QEasingCurve::InCubic);
            QObject::connect(anim, &QPropertyAnimation::finished, act, &QAction::trigger);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        else {
            act->trigger();
        }
    };

    if (act->isCheckable()) {
        /* Act is part of a group of destinations that switch */

        auto *but = new QRadioButton;
        makeButtonWrapText(but);

        but->setText(act->text());
        but->setToolTip(act->toolTip());
        but->setWhatsThis(act->whatsThis());
        but->setIcon(act->icon());
        but->setChecked(act->isChecked());

        connect(but, &QAbstractButton::toggled, [=](bool checked) {
            if (checked)
                triggerAction();
        });

        navV->addWidget(but);
    }
    else if (auto *wa = qobject_cast<QWidgetAction *>(act)) {
        navV->addWidget(wa->defaultWidget());
    }
    else {
        auto *but = new QToolButton;
        makeButtonWrapText(but);

        but->setDefaultAction(act);

        disconnect(but, &QAbstractButton::clicked, nullptr, nullptr);  // Remove the clicked() -> trigger() conneciton; we need to wedge the animation in between them
        connect(but, &QAbstractButton::clicked, triggerAction);

        navV->addWidget(but);
    }
}


static void wrapLines(QAbstractButton *that, int width) {
    QString word, line, result;
    for (QChar c : that->text().replace('\n', ' ') + ' ') {
        word += c;
        if (c.isSpace()) {
            if (!line.isEmpty() && that->fontMetrics().horizontalAdvance(line + word.trimmed()) > width) {
                result += line.trimmed() + '\n';
                line = word;
            } else {
                line += word;
            }
            word.clear();
        }
    }
    result += line.trimmed();
    that->setText(result.trimmed());
}

void makeButtonWrapText(QAbstractButton *that)
{
    onEvent(that, QEvent::Resize, [=](QEvent *event) {
        int controlElementWidth = that->sizeHint().width() - that->style()->itemTextRect(that->fontMetrics(), QRect(), Qt::TextShowMnemonic, false, that->text()).width();
        wrapLines(that, static_cast<QResizeEvent *>(event)->size().width() - controlElementWidth);
    });

    QSizePolicy policy = that->sizePolicy();
    // policy.setHorizontalPolicy(QSizePolicy::Preferred);
    that->setSizePolicy(policy);
    that->updateGeometry();
}
