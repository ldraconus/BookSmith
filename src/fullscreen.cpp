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

    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor postCursor = text->textCursor();
    QTextCursor cursor = text->textCursor();
    QTextBlockFormat blk = postCursor.blockFormat();
    QTextCharFormat is = cursor.charFormat();
    QTextCharFormat fmt;

    if (e->matches(QKeySequence::Cut)) text->cut();
    if (e->matches(QKeySequence::Copy)) text->copy();
    if (e->matches(QKeySequence::Paste)) {
        if (text->canPaste()) {
            text->paste();
            blk.setTextIndent(20.0);
            blk.setBottomMargin(10.0);
            postCursor.setBlockFormat(blk);
            text->setTextCursor(postCursor);
            MainWindow::getMainWindow()->change();
        }
    }
    if (e->matches(QKeySequence::Bold)) {
        fmt.setFontWeight(is.fontWeight() != QFont::Bold ? QFont::Bold : QFont::Normal);
        cursor.mergeCharFormat(fmt);
        text->mergeCurrentCharFormat(fmt);
        MainWindow::getMainWindow()->change();
    }
    if (e->matches(QKeySequence::Italic)) {
        fmt.setFontItalic(!is.fontItalic());
        cursor.mergeCharFormat(fmt);
        text->mergeCurrentCharFormat(fmt);
        MainWindow::getMainWindow()->change();
    }
    if (e->matches(QKeySequence::Underline)) {
        fmt.setFontUnderline(!is.fontUnderline());
        cursor.mergeCharFormat(fmt);
        text->mergeCurrentCharFormat(fmt);
        MainWindow::getMainWindow()->change();
    }
    if (e->matches(QKeySequence::Save)) MainWindow::getMainWindow()->saveAction();

    // [TODO] handle all of the edit keys
}

void FullScreen::textChangedAction()
{
    MainWindow::getMainWindow()->change();
}
