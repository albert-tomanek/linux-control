#ifndef THEMEWATCHER_H
#define THEMEWATCHER_H

#include <QObject>
#include <KConfigWatcher>

namespace Aero {
namespace Private {

class ThemeWatcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool themeIsAero READ themeIsAero NOTIFY themeChanged)

public:
    static ThemeWatcher *instance();

    bool themeIsAero();

signals:
    void themeChanged(bool isAero);

private:
    explicit ThemeWatcher(QObject *parent = nullptr);

    bool m_themeIsAero;     // We cache the value because it might be used in draw routines. We don't want to have to read the config file each time.
    KConfigWatcher::Ptr m_watcher;
};

} // namespace Private
} // namespace Aero

#endif // THEMEWATCHER_H
