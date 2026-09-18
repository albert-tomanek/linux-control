#pragma once

#include <QMainWindow>
#include <AeroQt/browser.h>

namespace Ui {
class NewMainWindow;
}

class NewMainWindow : public QMainWindow
{
    Q_OBJECT

    Aero::Browser *b;

public:
    explicit NewMainWindow(QWidget *parent = nullptr);
    ~NewMainWindow();

private:
    void makePages();
    void makeActions();

    Ui::NewMainWindow *ui;
};
