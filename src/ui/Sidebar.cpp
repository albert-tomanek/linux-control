#include "Sidebar.h"

Sidebar::Sidebar(int initialWidth, QWidget *parent) :
    QScrollArea(parent)
{
    this->setFixedWidth(initialWidth);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setFrameShape(QFrame::NoFrame);
    this->setWidgetResizable(true);
    this->setStyleSheet("QScrollArea { background: transparent; border: none; }");

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
    // auto *controlHome = new QLabel

    controlHome->setCursor(Qt::PointingHandCursor);
    // controlHome->setFlat(true);
    controlHome->setStyleSheet(
        "QToolButton         { border: none; background: transparent; color: #000000; text-align: left; padding: 0; font-size: 9pt; }"
        "QToolButton:hover   { color: #0033AA; text-decoration: underline; }"
    );
    navV->addWidget(controlHome);
    navV->addSpacing(16);

    this->setWidget(pane);
}
