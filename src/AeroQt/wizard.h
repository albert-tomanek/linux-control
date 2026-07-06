#ifndef WIZARD_QT_H
#define WIZARD_QT_H

#include <QWizard>
#include <QEvent>
#include <QSpacerItem>

namespace Aero {

class Wizard : public QWizard
{
public:
    explicit Wizard(QWidget *parent);

    static void convert(QWizard *that);     /// Call before shwoing, prefarably right after constructing
private:
    static QWidget *createHeader(QWizard *that);
    static void imposeWidgetStyle(QWizard *that, QWidget *newRoot);

    /*dynamic QWidget *"_Aero_originalRoot";*/   // The entire original QWizard UI is stored within a single child widget of `this`. We keep a reference to it since it stops being a direct child once we rejig things to make the wizard more Aero-y.
};

}

#endif
