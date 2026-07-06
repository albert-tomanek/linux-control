#include <QApplication>
#include "MainWindow.h"
#include "IconHelper.h"
#include "AeroQt/stylesheet.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("controlpanel");
    app.setApplicationName("controlpanel");
    // No setApplicationDisplayName: Qt appends it to every window/dialog title.
    app.setWindowIcon(themeIcon({"preferences-system"}));

    Aero::registerStylesheet(&app);

    MainWindow w;
    w.show();
    return app.exec();
}
