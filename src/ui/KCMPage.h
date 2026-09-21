#ifndef KCMPAGE_H
#define KCMPAGE_H

#include <KPluginMetaData>
#include <KCModuleLoader>
#include <KCModuleData>

#include <QWidget>
#include "PageBase.h"

class KCMPage : public PageBase
{
    Q_OBJECT

    QString m_kcmName;

public:
    Q_PROPERTY(QString kcmName READ kcmName)

    explicit KCMPage(Aero::Browser *browser, KPluginMetaData kcmMeta, QWidget *parent = nullptr);

    virtual QList<QAction *> sidebarLinks();
    virtual QList<QAction *> sidebarSeeAlso();

    inline QString kcmName() { return m_kcmName; }
};

#endif // KCMPAGE_H
