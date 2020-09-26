#include "fullscreen.h"
#include "ui_fullscreen.h"

FullScreen::FullScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FullScreen)
{
    ui->setupUi(this);

    connect(findChild<QTextEdit*>("textEdit"), SIGNAL(textChanged()), this, SLOT(textChangedAction()));

    MainWindow::Fullscreen& f = MainWindow::getMainWindow()->fullscreen();
    if (f.sizes_2.count() > 0) findChild<QSplitter*>("splitter_2")->setSizes(f.sizes_2);
    if (f.sizes.count() > 0) findChild<QSplitter*>("splitter")->setSizes(f.sizes);
}

FullScreen::~FullScreen()
{
    delete ui;
}

void FullScreen::resizeEvent(QResizeEvent* event)
{
   QDialog::resizeEvent(event);
   QSize mainSize = size();
   findChild<QSplitter*>("splitter_2")->resize(mainSize);
}

void FullScreen::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) this->close();

    // [TODO] handle all of the edit keys
}

void FullScreen::textChangedAction()
{
    MainWindow::getMainWindow()->change();
}
