#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QtWidgets>

class MainWindow;

namespace Aero {
    class Sidebar: public QScrollArea {
        Q_OBJECT

        QWidget     *textWrap, *m_seeAlsoLabel;
        QVBoxLayout *navV, *m_seeAlsoV;
        QFormLayout *m_itemsL;

        QGraphicsOpacityEffect *m_sidebarTextEffect;
        bool m_fadeInText, m_fadeOutText;

        QAction *m_goHome;

    public:
        void setFadeInText(bool b);
        void setFadeOutText(bool b);

    public:
        Sidebar(
            QAction *goHome = nullptr,
            int initialWidth = 0,
            QWidget *parent = nullptr
        );

        void addItem(QAction *act); // When an destination is clicked, it emits ::triggered()
        void addSeeAlso(QAction *act);

    private:
        void widgetForAction(QAction *act, QWidget *&widget, QWidget *&indicator);
    };
}

#endif // SIDEBAR_H
