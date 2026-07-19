#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QtWidgets>

class MainWindow;

class Sidebar: public QScrollArea {
    Q_OBJECT

    QWidget     *textWrap;
    QVBoxLayout *navV;

    QAction *m_goHome;

public:
    Sidebar(int initialWidth, QWidget *parent = nullptr);

    inline QAction *goHome() { return m_goHome; }

    friend class MainWindow;
};


#endif // SIDEBAR_H
