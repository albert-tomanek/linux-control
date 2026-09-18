#pragma once

#include <QMainWindow>
#include <AeroQt/browser.h>

namespace Ui {
class NewMainWindow;
}

class NewMainWindow : public QMainWindow
{
    Q_OBJECT

    Aero::Browser *m_br;

    QList<QAction *> m_kcmActions;

public:
    explicit NewMainWindow(QWidget *parent = nullptr);
    ~NewMainWindow();

private:
    void makePages();
    void makeActions();
    void populateKcms();

    QWidget *makeHomePage();

    Ui::NewMainWindow *ui;
};
