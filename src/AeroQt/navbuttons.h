#ifndef NAVBUTTONS_H
#define NAVBUTTONS_H

#include <QWidget>
#include <QMenu>

#include "private/imagebutton.h"
#include "util/props.h"

namespace Aero {

class NavButtons : public QWidget
{
    Q_OBJECT
    MAKE_PROPERTY(QMenu *, menu, Menu);

public:
    explicit NavButtons(QWidget *parent = nullptr);

    static QPushButton *makeForward(QWidget *parent = nullptr);
    static QPushButton *makeBack(QWidget *parent = nullptr);

    QPushButton *back() { return m_back; }
    QPushButton *forward() { return m_forward; }

    /* Menu button behavior:
     * - Invisible if no menu set
     * - Else interactivity mirrors menu()->isEnabled()
     */

    QPushButton *menuButton() { return m_menuButton; }

public:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void paintEvent(QPaintEvent*) override;

private:
    QPushButton *m_back;
    QPushButton *m_forward;
    QPushButton *m_menuButton;

    QPixmap background;
};

} // namespace Aero

#endif // NAVBUTTONS_H
