#pragma once

#include <QWidget>
#include <AeroQt/browser.h>
#include <AeroQt/util/props.h>

#include "PageId.h"

class PageBase : public QWidget
{
    Q_OBJECT

protected:
    Aero::Browser *m_browser;

public:
    explicit PageBase(Aero::Browser *browser, QWidget *parent = nullptr);

    void navigateTo(QString path);

    virtual QList<SidebarLink> sidebarLinks();
    virtual QList<SidebarLink> sidebarSeeAlso();
};

