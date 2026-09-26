#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include "PageBase.h"

#include <KPluginMetaData>

#include <AeroQt/actionpgph.h>
#include <AeroQt/util/flowlayout.h>

class CategPage : public PageBase
{
    Q_OBJECT

    FlowLayout *m_flow;

public:
    explicit CategPage(Aero::Browser *br, QString pathParent, bool expandCategs = false, QWidget *parent = nullptr);

    static QAction *registerRoot(Aero::Browser *br, QList<KPluginMetaData> allKcms);

protected:
    void addContextMenu(QWidget *w);
};

#endif // HOMEPAGE_H
