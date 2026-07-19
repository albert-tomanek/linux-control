#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QtWidgets>

class MainWindow;

class Sidebar: public QScrollArea {
    Q_OBJECT

    QWidget     *textWrap;
    QVBoxLayout *navV;

    QGraphicsOpacityEffect *m_sidebarTextEffect;

    QAction *m_goHome;

public:
    inline QAction *goHome() { return m_goHome; }

public:
    friend class MainWindow;

    Sidebar(int initialWidth, QWidget *parent = nullptr);

    void addDest(QAction *act); // When an destination is clicked, it emits ::triggered()
};

// https://stackoverflow.com/a/75379447
class LineWrappedRadioButton : public QRadioButton {
    void wrapLines(int width);
protected:
    virtual void resizeEvent(QResizeEvent *event);
public:
    LineWrappedRadioButton(QWidget *parent = nullptr) : LineWrappedRadioButton(QString(), parent) { }
    LineWrappedRadioButton(const QString &text, QWidget *parent = nullptr);
    virtual QSize minimumSizeHint() const { return QSize(QRadioButton().minimumSizeHint().width(), sizeHint().height()); }
};




#endif // SIDEBAR_H
