#include "Sidebar.h"

Sidebar::Sidebar(int initialWidth, QWidget *parent) :
    QScrollArea(parent)
{
    this->setFixedWidth(initialWidth);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setFrameShape(QFrame::NoFrame);
    this->setWidgetResizable(true);
    this->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QToolButton         { border: none; background: transparent; color: #000000; text-align: left; padding: 0; font-size: 9pt; }"
        "QToolButton:hover   { color: #0033AA; text-decoration: underline; }"
        "QToolButton:hover:disabled { text-decoration: none; }"
    );

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
    // Static sidebar (shown at full opacity). The effect is kept so that
    // navigating into a subpage can fade this text out first.
    m_sidebarTextEffect = new QGraphicsOpacityEffect(textWrap);
    textWrap->setGraphicsEffect(m_sidebarTextEffect);
    m_sidebarTextEffect->setOpacity(1.0);
}

void Sidebar::addDest(QAction *act)
{
    if (act->isCheckable()) {   // Act is part of a group of destinations that switch
        auto *but = new LineWrappedRadioButton;

        but->setText(act->text());
        but->setToolTip(act->toolTip());
        but->setIcon(act->icon());
        but->setChecked(act->isChecked());

        connect(but, &QAbstractButton::toggled, [=](bool checked) {
            if (checked) {
                auto *anim = new QPropertyAnimation(m_sidebarTextEffect, "opacity");
                anim->setStartValue(1.0);
                anim->setEndValue(0.0);
                anim->setDuration(300);
                anim->setEasingCurve(QEasingCurve::InCubic);
                QObject::connect(anim, &QPropertyAnimation::finished, act, &QAction::trigger);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        });

        navV->addWidget(but);
    }
}


void LineWrappedRadioButton::wrapLines(int width) {
    QString word, line, result;
    for (QChar c : text().replace('\n', ' ') + ' ') {
        word += c;
        if (c.isSpace()) {
            if (!line.isEmpty() && fontMetrics().horizontalAdvance(line + word.trimmed()) > width) {
                result += line.trimmed() + '\n';
                line = word;
            } else {
                line += word;
            }
            word.clear();
        }
    }
    result += line.trimmed();
    setText(result.trimmed());
}

void LineWrappedRadioButton::resizeEvent(QResizeEvent *event) {
    int controlElementWidth = sizeHint().width() - style()->itemTextRect(fontMetrics(), QRect(), Qt::TextShowMnemonic, false, text()).width();
    wrapLines(event->size().width() - controlElementWidth);
    QRadioButton::resizeEvent(event);
}

LineWrappedRadioButton::LineWrappedRadioButton(const QString &text, QWidget *parent) : QRadioButton(text, parent) {
    QSizePolicy policy = sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Preferred);
    setSizePolicy(policy);
    updateGeometry();
}
