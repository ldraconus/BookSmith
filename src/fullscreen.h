#ifndef FULLSCREEN_H
#define FULLSCREEN_H

#include <QDialog>
#include <QKeyEvent>
#include <QResizeEvent>

#include "mainwindow.h"

namespace Ui {
class FullScreen;
}

class FullScreen : public QDialog
{
    Q_OBJECT

public:
    explicit FullScreen(QWidget *parent = 0);
    ~FullScreen();

    void resizeEvent(QResizeEvent* event);
    void keyReleaseEvent(QKeyEvent *e);

private:
    Ui::FullScreen *ui;

public slots:
    void textChangedAction();
};

#endif // FULLSCREEN_H
