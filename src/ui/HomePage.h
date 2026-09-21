#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include "PageBase.h"

#include <AeroQt/actionpgph.h>
#include <AeroQt/util/flowlayout.h>

class HomePage : public PageBase
{
    Q_OBJECT

    FlowLayout *m_flow;

    QMap<QString, Aero::ActionPgph *> m_categs;

public:
    explicit HomePage(Aero::Browser *br, QWidget *parent = nullptr);

signals:
private:
    void makeCategs();
};

#endif // HOMEPAGE_H
