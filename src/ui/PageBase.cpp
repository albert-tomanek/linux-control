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

QList<QAction *> PageBase::sidebarLinks()
{
    return {};
}

QList<QAction *> PageBase::sidebarSeeAlso()
{
    return {};
}


QAction *findPageForKcm(Aero::Browser *br, QString name)
{
    for (auto path: br->allPaths())
        if (path.split("/").last() == name)
            return br->actionForPath(path);
    return nullptr;
}
