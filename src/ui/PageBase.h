#pragma once

#include <QWidget>
#include <AeroQt/browser.h>
#include <AeroQt/util/props.h>

#include "PageId.h"
#include <AeroQt/page.h>

// Pages are not cached; they get created anew every time the browser navigates to their location

class PageBase : public QWidget
{
    Q_OBJECT

protected:
    Aero::Browser *m_browser;

public:
    explicit PageBase(Aero::Browser *browser, QWidget *parent = nullptr);

    void navigateTo(QString path);  // This lets you control the page browser from within a displayed page

    virtual QList<QAction *> sidebarLinks();
    virtual QList<QAction *> sidebarSeeAlso();

    /** All subclasses must have these, they're used by NewMainWindow.cpp to figure out where to place the page.
     *  inline static QString path = "...";
     *  inline static void initAction(QAction *ac)
	{
	}  // Init text, tooltips, etc in the action that opens this page
     */
};

QAction *findPageForKcm(Aero::Browser *br, QString name);

