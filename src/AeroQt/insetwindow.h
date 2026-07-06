#ifndef INSETWINDOW_H
#define INSETWINDOW_H

#include <QWidget>

namespace Aero {

/// Call from window constructor. Do not change Aero windows' `setContentsMargins()`, it will interfere with space allocation for the inset glass widgets.

void makeInsetWindow(
    QWidget *window,
    QWidget *contents,      /// Ignored for QMainWindows
    QWidget *top = nullptr,
    QWidget *bottom = nullptr
);

} // namespace Aero

#endif // INSETWINDOW_H
