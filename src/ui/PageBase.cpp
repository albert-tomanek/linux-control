#include "PageBase.h"

PageBase::PageBase(Aero::Browser *browser, QWidget *parent) :
    QWidget(parent)
{
    m_browser = browser;
}

void PageBase::navigateTo(QString path)
{
    m_browser->navigateTo(path);
}

QList<SidebarLink> PageBase::sidebarLinks()
{
    return {};
}

QList<SidebarLink> PageBase::sidebarSeeAlso()
{
    return {};
}


