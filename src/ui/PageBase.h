#pragma once

#include <QWidget>
#include <AeroQt/browser.h>
#include <AeroQt/util/props.h>

#include "PageId.h"
#include <AeroQt/page.h>

class PageBase : public QWidget
{
    Q_OBJECT

protected:
    Aero::Browser *m_browser;

public:
    explicit PageBase(Aero::Browser *browser, QWidget *parent = nullptr);

    void navigateTo(QString path);

    virtual QList<QAction *> sidebarLinks();
    virtual QList<QAction *> sidebarSeeAlso();

    /** Do this in subclasses to determine where you want the page to be placed by NewMainWindow.cpp
     * Q_CLASSINFO("PagePath", "/foo/bar");
     */
};

QAction *findPageForKcm(Aero::Browser *br, QString name);

