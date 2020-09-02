#include "fullscreen.h"
#include "ui_fullscreen.h"

FullScreen::FullScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FullScreen)
{
    ui->setupUi(this);

    connect(findChild<QTextEdit*>("textEdit"), SIGNAL(textChanged()), this, SLOT(textChangedAction()));
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
}

void FullScreen::textChangedAction()
{
    MainWindow::getMainWindow()->change();
}
