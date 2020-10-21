#include "mainwindow.h"
#include "fullscreen.h"
#include "tagsdialog.h"
#include "finddialog.h"
#include "help.h"
#include "replacedialog.h"
#include "ui_mainwindow.h"
#include "util.h"

#include <QDir>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QToolBar>
#include <QMimeData>
#include <QImageReader>

MainWindow* MainWindow::_mainWindow = nullptr;

class TextEdit : public QTextEdit
{
public:
    bool canInsertFromMimeData(const QMimeData* source) const
    {
        return source->hasImage() || source->hasUrls() ||
            QTextEdit::canInsertFromMimeData(source);
    }

    void insertFromMimeData(const QMimeData* source)
    {
        if (source->hasImage())
        {
            static int i = 1;
            QUrl url(QString("dropped_image_%1").arg(i++));
            dropImage(url, qvariant_cast<QImage>(source->imageData()));
        }
        else if (source->hasUrls())
        {
            foreach (QUrl url, source->urls())
            {
                QFileInfo info(url.toLocalFile());
                if (QImageReader::supportedImageFormats().contains(info.suffix().toLower().toLatin1()))
                    dropImage(url, QImage(info.filePath()));
                else
                    dropTextFile(url);
            }
        }
        else
        {
            QTextEdit::insertFromMimeData(source);
        }
    }

private:
    void dropImage(const QUrl& url, const QImage& image)
    {
        if (!image.isNull())
        {
            document()->addResource(QTextDocument::ImageResource, url, image);
            textCursor().insertImage(url.toString());
        }
    }

    void dropTextFile(const QUrl& url)
    {
        QFile file(url.toLocalFile());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            textCursor().insertText(file.readAll());
    }
};

void MainWindow::Search::init(FindDialog::type r)
{
    _stack.clear();
    _range = r;
    QTreeWidget* tree = MainWindow::_mainWindow->findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    int currentIdx = current->data(0, Qt::ItemDataRole::UserRole).toInt();

    switch (_range) {
    case FindDialog::Selection:
        {
            QTextEdit* text = MainWindow::_mainWindow->findChild<QTextEdit*>("textEdit");
            QTextCursor cursor = text->textCursor();
            int offset = cursor.selectionStart();
            int end = cursor.selectionEnd();
            _start.nil();
            _start.scene(currentIdx);
            _start.offset(offset);
            _stop.scene(currentIdx);
            _stop.offset(end);
        }
        break;
    case FindDialog::Scene:
    case FindDialog::SceneChildren:
        _start.nil();
        _start.scene(currentIdx);
        break;
    case FindDialog::SiblingChildren:
        {
            QTreeWidgetItem* parent = current->parent();
            int parentIdx = parent->data(0, Qt::ItemDataRole::UserRole).toInt();
            QList<int> children = MainWindow::_mainWindow->getChildren(parentIdx);
            _start.nil();
            _start.scene(children[0]);
        }
        break;
    case FindDialog::All:
        {
            QTreeWidgetItem* root = tree->topLevelItem(0);
            int rootIdx = root->data(0, Qt::ItemDataRole::UserRole).toInt();
            _start.nil();
            _start.scene(rootIdx);
        }
        break;
    }

    _current = _start;
    _stack.append(_current.scene());
}


MainWindow::Search::Search(QString& l, FindDialog::type r): _look(l), _with("")
{
    init(r);
}

MainWindow::Search::Search(QString& l, FindDialog::type r, QString& w): _look(l), _with(w)
{
    init(r);
}

MainWindow::Position MainWindow::Search::findNextChild(Position current)
{
    int at = current.scene();
    QList<int> children = MainWindow::_mainWindow->getChildren(at);
    if (!children.isEmpty()) {
        _stack.append(children[0]);
        return Position(children[0]);
    }

    _stack.pop_back();

    while (!_stack.isEmpty()) {
        int parentIdx = MainWindow::_mainWindow->getParent(at);
        children = MainWindow::_mainWindow->getChildren(parentIdx);
        int cnt = children.count() - 1;
        for (int x = 0; x < cnt; ++x) if (children[x] == at) {
            _stack.append(children[x + 1]);
            return Position(children[x + 1]);
        }
        at = _stack.last();
        _stack.pop_back();
    }

    return Position();
}

MainWindow::Position MainWindow::Search::findNext()
{
    QTextEdit work;
    int idx = _current.scene();
    int offset = _current.offset();
    Scene& scene = MainWindow::_mainWindow->_scenes[idx];
    _current.nil();
    work.setHtml(scene._doc);
    for (; ; ) {
        int start = offset + 1;
        QString txt = work.toPlainText();
        int at = txt.indexOf(_look, start);
        if (at == -1) { // nothing left in this scene, get the next scene (if any)
            switch (_range) {
            case FindDialog::Selection:
            case FindDialog::Scene:
                return Position();
            case FindDialog::SceneChildren:
            case FindDialog::SiblingChildren:
            case FindDialog::All:
                {
                    Position next = findNextChild(Position(idx));
                    if (next.unset()) return Position();
                    idx = next.scene();
                    Scene& scene = MainWindow::_mainWindow->_scenes[idx];
                    work.setHtml(scene._doc);
                    offset = -1;
                }
                break;
            }
        }
        else {
            if (_range == FindDialog::Selection) {
                if (at > _stop.offset() - _look.count() + 1) return Position();
            }
            return Position(idx, at);
        }
    }
}

void MainWindow::createToolBarItem(QToolBar* tb, const QString& icon, const QString& name, const QString& tip, const char* signal, const char* slot)
{
    QAction* action = tb->addAction(QIcon(icon), name);
    action->setToolTip(tip);
    action->connect(action, signal, this, slot);
}

MainWindow::MainWindow(const QString& file, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _mainWindow = this;
    _dirty = false;

    connect(findChild<QAction*>("actionExit"),        SIGNAL(triggered()), this, SLOT(checkAction()));
    connect(findChild<QAction*>("actionFull_Screen"), SIGNAL(triggered()), this, SLOT(fullScreenAction()));
    connect(findChild<QAction*>("actionNew"),         SIGNAL(triggered()), this, SLOT(newAction()));
    connect(findChild<QAction*>("actionOpen"),        SIGNAL(triggered()), this, SLOT(openAction()));
    connect(findChild<QAction*>("actionSave"),        SIGNAL(triggered()), this, SLOT(saveAction()));
    connect(findChild<QAction*>("actionSave_As"),     SIGNAL(triggered()), this, SLOT(saveAsAction()));
    connect(findChild<QAction*>("actionNewScene"),    SIGNAL(triggered()), this, SLOT(newSceneAction()));
    connect(findChild<QAction*>("actionDelete"),      SIGNAL(triggered()), this, SLOT(deleteAction()));
    connect(findChild<QAction*>("actionMove_In"),     SIGNAL(triggered()), this, SLOT(moveInAction()));
    connect(findChild<QAction*>("actionMove_Out"),    SIGNAL(triggered()), this, SLOT(moveOutAction()));
    connect(findChild<QAction*>("actionMove_Up"),     SIGNAL(triggered()), this, SLOT(moveUpAction()));
    connect(findChild<QAction*>("actionMove_Down"),   SIGNAL(triggered()), this, SLOT(moveDownAction()));
    connect(findChild<QAction*>("actionCut"),         SIGNAL(triggered()), this, SLOT(cutAction()));
    connect(findChild<QAction*>("actionCopy"),        SIGNAL(triggered()), this, SLOT(copyAction()));
    connect(findChild<QAction*>("actionPaste"),       SIGNAL(triggered()), this, SLOT(pasteAction()));
    connect(findChild<QAction*>("actionFind"),        SIGNAL(triggered()), this, SLOT(findAction()));
    connect(findChild<QAction*>("actionReplace"),     SIGNAL(triggered()), this, SLOT(replaceAction()));
    connect(findChild<QAction*>("actionBold"),        SIGNAL(triggered()), this, SLOT(boldAction()));
    connect(findChild<QAction*>("actionItalic"),      SIGNAL(triggered()), this, SLOT(italicAction()));
    connect(findChild<QAction*>("actionUnderline"),   SIGNAL(triggered()), this, SLOT(underlineAction()));
    connect(findChild<QAction*>("actionLeft"),        SIGNAL(triggered()), this, SLOT(leftAction()));
    connect(findChild<QAction*>("actionCenter"),      SIGNAL(triggered()), this, SLOT(centerAction()));
    connect(findChild<QAction*>("actionRight"),       SIGNAL(triggered()), this, SLOT(rightAction()));
    connect(findChild<QAction*>("actionJustify"),     SIGNAL(triggered()), this, SLOT(fullJustifyAction()));
    connect(findChild<QAction*>("actionIndent"),      SIGNAL(triggered()), this, SLOT(indentAction()));
    connect(findChild<QAction*>("actionOutdent"),     SIGNAL(triggered()), this, SLOT(outdentAction()));
    connect(findChild<QAction*>("actionAbout"),       SIGNAL(triggered()), this, SLOT(aboutAction()));
    connect(findChild<QAction*>("actionODF"),         SIGNAL(triggered()), this, SLOT(odfAction()));
    connect(findChild<QAction*>("actionPDF"),         SIGNAL(triggered()), this, SLOT(pdfAction()));
    connect(findChild<QAction*>("actionPlain_Text"),  SIGNAL(triggered()), this, SLOT(textAction()));
    connect(findChild<QAction*>("actionEPUB"),        SIGNAL(triggered()), this, SLOT(epubAction()));
    connect(findChild<QAction*>("actionHelp"),        SIGNAL(triggered()), this, SLOT(helpAction()));

    connect(findChild<QTreeWidget*>("treeWidget"), SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChangedAction(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(findChild<QTreeWidget*>("treeWidget"), SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),               this, SLOT(itemDoubleClickedAction(QTreeWidgetItem*,int)));

    connect(findChild<QTextEdit*>("textEdit"), SIGNAL(textChanged()), this, SLOT(textChangedAction()));

    connect(findChild<QMenu*>("menuFile"),  SIGNAL(aboutToShow()), this, SLOT(fileShowAction()));
    connect(findChild<QMenu*>("menuScene"), SIGNAL(aboutToShow()), this, SLOT(sceneShowAction()));
    connect(findChild<QMenu*>("menuEdit"),  SIGNAL(aboutToShow()), this, SLOT(editShowAction()));

    _exeDir = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();

#ifdef Q_OS_MACOS
    // take three dirs off _exeDir
    int pos;
    for (int i = 0; i < 3 && (pos = _exeDir.lastIndexOf("/")) != -1; ++i) _exeDir = _exeDir.left(pos);
#endif

    QToolBar* toolbar = findChild<QToolBar*>("mainToolBar");
    createToolBarItem(toolbar, _exeDir + "/gfx/blank-file-16.ico",                      "New",        "New Book (Ctrl-N)",       SIGNAL(triggered()), SLOT(newAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/folder-3-16.ico",                        "Open",       "Open Book (Ctrl-O)",      SIGNAL(triggered()), SLOT(openAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/downloads-16.ico",                       "Save",       "Save Book (Ctrl-S)",      SIGNAL(triggered()), SLOT(saveAction()));
    toolbar->addSeparator();
    createToolBarItem(toolbar, _exeDir + "/gfx/bold-16.ico",                            "Bold",       "Bold Text (Ctrl-B)",      SIGNAL(triggered()), SLOT(boldAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/italic-16.ico",                          "Italic",     "Italic Text (Ctrl-I)",    SIGNAL(triggered()), SLOT(italicAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/Icons8-Windows-8-Editing-Underline.ico", "Underline",  "Underline Text (Ctrl-U)", SIGNAL(triggered()), SLOT(underlineAction()));
    toolbar->addSeparator();
    createToolBarItem(toolbar, _exeDir + "/gfx/alignment+left+text+icon_16.ico",        "Left",       "Left Text (Alt-L)",       SIGNAL(triggered()), SLOT(leftAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/alignment+center+text+icon_16.ico",      "Center",     "Center Text (Alt-C)",     SIGNAL(triggered()), SLOT(centerAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/alignment+right+text+icon_16.ico",       "Right",      "Right Text (Alt-R)",      SIGNAL(triggered()), SLOT(rightAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/alignment+justify+text+icon_16.ico",     "Justify",    "Justify Text (Alt-J)",    SIGNAL(triggered()), SLOT(fullJustifyAction()));
    toolbar->addSeparator();
    createToolBarItem(toolbar, _exeDir + "/gfx/format+indent+increase_16.ico",          "Indent",     "Indent (Alt-])",          SIGNAL(triggered()), SLOT(indentAction()));
    createToolBarItem(toolbar, _exeDir + "/gfx/format+indent+decrease16.ico",           "Outdent",    "Outdent (Alt-[)",         SIGNAL(triggered()), SLOT(outdentAction()));
    toolbar->addSeparator();
    createToolBarItem(toolbar, _exeDir + "/gfx/fullscreen_16.ico",                      "Fullscreen", "Full Screen (F11)",       SIGNAL(triggered()), SLOT(fullScreenAction()));

    QApplication::setWindowIcon(QIcon(_exeDir + "/gfx/BookSmith.ico"));

    QSplitter* splitter = findChild<QSplitter*>("splitter");
    QSize mainSize = size();
    QSize splitterSize = splitter->size();
    _diff.setHeight(mainSize.height() - splitterSize.height());
    _diff.setWidth(mainSize.width() - splitterSize.width());

    QList<int> sizes = splitter->sizes();
    sizes[0] = 200;
    sizes[1] = splitterSize.width() - 200;
    splitter->setSizes(sizes);

    _dir = QDir::homePath();

    _wcLabel = new QLabel("0/0");
    QStatusBar* bar = findChild<QStatusBar*>("statusBar");
    bar->addWidget(_wcLabel);

    _totalWc = 0;

    Util setup(ok(), okcancel(), yesno(), yesnocancel(), question(), statement());

    newAction();
    if (file != "") {
        printf("File = \"%s\"\n", file.toStdString().c_str());
        open(file);
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

static QTreeWidgetItem* getItemByItemIndex(QTreeWidgetItem* item, int idx)
{
    if (item->data(0, Qt::ItemDataRole::UserRole).toInt() == idx) return item;
    int count = item->childCount();
    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem* found = getItemByItemIndex(item->child(i), idx);
        if (found) return found;
    }
    return nullptr;
}

QTreeWidgetItem* MainWindow::getItemByIndex(int idx)
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* root = tree->topLevelItem(0);
    return getItemByItemIndex(root, idx);
}

QList<int> MainWindow::getChildren(int idx)
{
    QList<int> children;
    QTreeWidgetItem* item = getItemByIndex(idx);
    int count = item->childCount();
    for (int i = 0; i < count; ++i) children.append(item->child(i)->data(0, Qt::ItemDataRole::UserRole).toInt());
    return children;
}

int MainWindow::getParent(int idx)
{
    QTreeWidgetItem* item = getItemByIndex(idx);
    QTreeWidgetItem* parent = item->parent();
    return parent->data(0, Qt::ItemDataRole::UserRole).toInt();
}

QJsonObject MainWindow::sceneToObject(const Scene& scene)
{
    QJsonObject obj;
    obj.insert("name", scene._name);
    obj.insert("root", scene._root);
    obj.insert("doc", scene._doc);
    obj.insert("wc", scene._wc);
    QJsonArray tags;
    for (auto tag: scene._tags) tags.append(tag);
    obj.insert("tags", tags);
    return obj;
}

QJsonObject MainWindow::itemToObject(const QTreeWidgetItem* scene)
{
    int idx = scene->data(0, Qt::ItemDataRole::UserRole).toInt();
    QJsonObject sceneObject = sceneToObject(_scenes[idx]);
    QJsonArray children;
    int num = scene->childCount();
    for (int i = 0; i < num; ++i) children.append(itemToObject(scene->child(i)));
    sceneObject.insert("children", children);
    return sceneObject;
}

static QJsonObject widgetToObject(QWidget* w)
{
    QJsonObject widget;
    QRect r = w->geometry();
    widget.insert("top", r.top());
    widget.insert("left", r.left());
    widget.insert("bottom", r.bottom());
    widget.insert("right", r.right());
    return widget;
}

static QJsonArray listToArray(QList<int> x)
{
    QJsonArray a;
    for (int i = 0; i < x.count(); ++i) a.append(x[i]);
    return a;
}

static QJsonObject dialogToObject(MainWindow::Dialog x)
{
    QJsonObject o;
    o.insert("left", x.left);
    o.insert("top", x.top);
    return o;
}

bool MainWindow::save()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* rootItem = tree->topLevelItem(0);
    int idx = rootItem->data(0, Qt::ItemDataRole::UserRole).toInt();
    if (_scenes[idx]._name == "") if (!saveAs()) return false;

    updateSceneWithEdits();

    QJsonObject root;
    root.insert("root", itemToObject(rootItem));
    QJsonObject windows;
    QJsonObject mainwindow = widgetToObject((QWidget*) this);
    mainwindow.insert("splitter", (findChild<QSplitter*>("splitter")->sizes())[0]);
    windows.insert("mainwindow", mainwindow);
    QJsonObject fullscr;
    fullscr.insert("sizes", listToArray(fullscreen().sizes));
    fullscr.insert("sizes_2", listToArray(fullscreen().sizes_2));
    windows.insert("fullscreen", fullscr);
    windows.insert("finddialog", dialogToObject(finddialog()));
    windows.insert("replacedialog", dialogToObject(replacedialog()));
    windows.insert("tagdialog", dialogToObject(tagdialog()));
    windows.insert("ok", dialogToObject(ok()));
    windows.insert("okcancel", dialogToObject(okcancel()));
    windows.insert("yesno", dialogToObject(yesno()));
    windows.insert("yesnocancel", dialogToObject(yesnocancel()));
    windows.insert("question", dialogToObject(question()));
    windows.insert("statement", dialogToObject(statement()));
    QJsonObject top;
    top.insert("document", root);
    top.insert("windows", windows);
    QJsonDocument json;
    json.setObject(top);
    QFile file(_dir + "/" + _scenes[idx]._name + ".novel");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
    QTextStream out(&file);
    out << json.toJson();
    file.close();
    _dirty = false;
    return true;
}

bool MainWindow::saveAs()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save File", _dir, "Novels (*.novel)");
    if (filename.isEmpty()) return false;
    int ext = filename.lastIndexOf(".novel");
    if (ext != -1) filename = filename.left(ext);
    int sep = filename.lastIndexOf("/");
    if (sep != -1) {
        _dir = filename.left(sep + 1);
        filename = filename.mid(sep + 1);
    }

    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* rootItem = tree->topLevelItem(0);
    rootItem->setText(0, filename);
    int idx = rootItem->data(0, Qt::ItemDataRole::UserRole).toInt();
    _scenes[idx]._name = filename;
    return true;
}

static void objectToWidget(const QJsonObject& widget, QWidget* w)
{
    QRect r;
    r.setTop(widget["top"].toInt());
    r.setLeft(widget["left"].toInt());
    r.setBottom(widget["bottom"].toInt());
    r.setRight(widget["right"].toInt());
    w->setGeometry(r);
}

static QList<int> arrayToList(QJsonArray a)
{
    QList<int> lst;
    for (int i = 0; i < a.count(); ++i) lst.append(a[i].toInt());
    return lst;
}

static void toDialog(const QJsonObject& windows, const QString& name, MainWindow::Dialog& dlg)
{
    if (windows.contains(name)) {
        const QJsonObject& dialog = windows[name].toObject();
        dlg.left = dialog["left"].toInt();
        dlg.top = dialog["top"].toInt();
    }
}

bool MainWindow::open(const QString& fname)
{
    QString filename = fname;
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");

    int ext = filename.lastIndexOf(".novel");
    if (ext != -1) filename = filename.left(ext);
    int sep = filename.lastIndexOf("/");
    if (sep != -1) {
        _dir = filename.left(sep + 1);
        filename = filename.mid(sep + 1);
    }
    sep = filename.lastIndexOf("\\");
    if (sep != -1) {
        _dir = filename.left(sep + 1);
        filename = filename.mid(sep + 1);
    }

    QFile file(_dir + "/" + filename + ".novel");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QByteArray data(file.readAll());
    QString jsonStr(data);
    file.close();
    QJsonDocument json = QJsonDocument::fromJson(jsonStr.toUtf8());

    // scan the json structure in for valid novel
    const QJsonObject& top = json.object();
    if (!top.contains("document")) return false;

    const QJsonObject& doc = top["document"].toObject();
    const QJsonObject& root = doc["root"].toObject();
    tree->clear();
    _scenes.clear();
    tree->addTopLevelItem(objectToItem(root, _totalWc));

    if (top.contains("windows")) {
        const QJsonObject& windows = top["windows"].toObject();
        QJsonObject mainwindow = windows["mainwindow"].toObject();
        objectToWidget(mainwindow, (QWidget*) this);
        if (mainwindow.contains("splitter")) {
            int w = mainwindow["splitter"].toInt();
            QSplitter* splitter = findChild<QSplitter*>("splitter");
            QSize splitterSize = splitter->size();
            QList<int> sizes = splitter->sizes();
            sizes[0] = w;
            sizes[1] = splitterSize.width() - w;
            splitter->setSizes(sizes);
        }
        if (windows.contains("fullscreen")) {
            const QJsonObject& fullscr = windows["fullscreen"].toObject();
            struct MainWindow::Fullscreen& full = fullscreen();
            full.sizes = arrayToList(fullscr["sizes"].toArray());
            full.sizes_2 = arrayToList(fullscr["sizes_2"].toArray());
        }
        toDialog(windows, "finddialog", finddialog());
        toDialog(windows, "replacedialog", replacedialog());
        toDialog(windows, "tagdialog", tagdialog());
        toDialog(windows, "ok", ok());
        toDialog(windows, "okcancel", okcancel());
        toDialog(windows, "yesno", yesno());
        toDialog(windows, "yesnocancel", yesnocancel());
        toDialog(windows, "question", question());
        toDialog(windows, "statement", statement());
    }

    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->clear();
    text->setEnabled(false);
    _dirty = false;
    return true;
}

bool MainWindow::checkClose()
{
    if (_dirty) {
        switch (Util::YesNoCancel("Do you want to save your changes first?", "The current document has been changed!")) {
        case QMessageBox::Yes: if (save()) break; else return false;
        case QMessageBox::No:  break;
        default:               return false;
        }
    }
    else {
        switch (Util::YesNo("Are you sure you want to quit?", "")) {
        case QMessageBox::Yes: break;
        default:               return false;
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (checkClose()) event->accept(); else event->ignore();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   QSize mainSize = size();
   mainSize.setHeight(mainSize.height() - _diff.height());
   mainSize.setWidth(mainSize.width() - _diff.width());
   findChild<QSplitter*>("splitter")->resize(mainSize);
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (e->matches(QKeySequence::Save)) saveAction();
}

void MainWindow::aboutAction()
{
    Util::Statement("<h1><center><b>About BookSmith</b></center></h1>"
                    "<p><i>BookSmith</i> is a program designed to make writing books easier without a lot of distracting extras. "
                    "It also features the ability to export the finished book as, at a minimum, a PDF, an epub, a ODF, or a text file.</p>"
                    "<p>This program was writting by Christopher Martin Olson using the Qt multi-OS framework 5.x</p>"
                    "<p>It also uses icons for the menus and toolbar from across the internet. If you believe that any of them are being used in violation of"
                    "copyright laws, please let the author know and he will seek a different icon immediately.  Thank you!</p>"
                    "<p>Thank you to my wife, Nichola, for putting up with me writing this beast, and even encouraging me to finish it.</p>"
                    "<p>Thank you to NaNoWriMo (look it up!) for giving me permission to write badly, because writing is something I've always badly wanted to do!</p>"
                    "<p>Above all, thank you for giving this program a chance!  I hope you find it as useful a writing tool as I do!</p>");
}

void MainWindow::boldAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor cursor = text->textCursor();
    QTextCharFormat is = cursor.charFormat();
    QTextCharFormat fmt;
    fmt.setFontWeight(is.fontWeight() != QFont::Bold ? QFont::Bold : QFont::Normal);
    cursor.mergeCharFormat(fmt);
    text->mergeCurrentCharFormat(fmt);
    _dirty = true;
}

void MainWindow::centerAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->setAlignment(Qt::AlignCenter);
}

void MainWindow::checkAction()
{
    close();
}

void MainWindow::copyAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->copy();
}

void MainWindow::currentItemChangedAction(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    bool wasDirty = _dirty;
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    if (current) {
        int currentIdx = current->data(0, Qt::ItemDataRole::UserRole).toInt();
        int previousIdx = 0;
        if (previous) previousIdx = previous->data(0, Qt::ItemDataRole::UserRole).toInt();
        if (currentIdx < _scenes.size()) {
            Scene& currentScene = _scenes[currentIdx];
            if (previousIdx < _scenes.size()) {
                Scene& previousScene = _scenes[previousIdx];
                if (previous && !previousScene._root) previousScene._doc = text->toHtml();
                else text->setEnabled(true);
            }

            if (!currentScene._root) {
                text->setHtml(currentScene._doc);
                if (currentScene._doc.isEmpty()) {
                    QTextCursor cursor = text->textCursor();
                    QTextBlockFormat blk = cursor.blockFormat();
                    blk.setTextIndent(20.0);
                    blk.setBottomMargin(10.0);
                    cursor.setBlockFormat(blk);
                    text->setTextCursor(cursor);
                }
                _dirty = wasDirty;
                return;
            }
        }
    }

    text->clear();
    text->setEnabled(false);
    _dirty = wasDirty;
}

void MainWindow::cutAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->cut();
}

void MainWindow::deleteAction()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    QTreeWidgetItem* parent = current->parent();
    if (parent == nullptr) return;
    switch (Util::YesNo("Abort?", "This action cannot be undone!")) {
    case QMessageBox::Yes: return;
    default:               break;
    }
    parent->removeChild(current);
}

void MainWindow::editShowAction()
{
    static QAction* pasteAction = nullptr;
    static QAction* boldAction = nullptr;
    static QAction* italicAction = nullptr;
    static QAction* underlineAction = nullptr;
    static QAction* leftAction = nullptr;
    static QAction* centerAction = nullptr;
    static QAction* rightAction = nullptr;
    static QAction* justifyAction = nullptr;
    static QAction* outdentAction = nullptr;
    static QTextEdit* text = nullptr;

    if (!pasteAction) {
        pasteAction = findChild<QAction*>("actionPaste");
        boldAction = findChild<QAction*>("actionBold");
        italicAction = findChild<QAction*>("actionItalic");
        underlineAction = findChild<QAction*>("actionUnderline");
        leftAction = findChild<QAction*>("actionLeft");
        centerAction = findChild<QAction*>("actionCenter");
        rightAction = findChild<QAction*>("actionRight");
        justifyAction = findChild<QAction*>("actionJustify");
        outdentAction = findChild<QAction*>("actionOutdent");
        text = findChild<QTextEdit*>("textEdit");
    }

    QTextCursor cursor = text->textCursor();
    QTextCharFormat is = cursor.charFormat();
    QTextBlockFormat blk = cursor.blockFormat();
    pasteAction->setEnabled(text->canPaste());
    boldAction->setChecked(is.fontWeight() == QFont::Bold);
    italicAction->setChecked(is.fontItalic());
    underlineAction->setChecked(is.fontUnderline());
    leftAction->setChecked(text->alignment() == Qt::AlignLeft);
    centerAction->setChecked(text->alignment() == Qt::AlignCenter);
    rightAction->setChecked(text->alignment() == Qt::AlignRight);
    justifyAction->setChecked(text->alignment() == Qt::AlignJustify);
    outdentAction->setEnabled(blk.indent() != 0);
}

void MainWindow::epubAction()
{
    exportAs("EPUB");
}

void MainWindow::fileShowAction()
{
    QAction* saveAction = findChild<QAction*>("actionSave");
    saveAction->setEnabled(_dirty);
}

void MainWindow::updateSceneWithEdits()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    int currentIdx = -1;
    if (current) currentIdx = current->data(0, Qt::ItemDataRole::UserRole).toInt();
        if (currentIdx < _scenes.size()) {
            Scene& currentScene = _scenes[currentIdx];
            if (current && !currentScene._root) currentScene._doc = text->toHtml();
        }
}


void MainWindow::findAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor cursor = text->textCursor();
    updateSceneWithEdits();

    FindDialog find(cursor.hasSelection());
    find.show();

    int res = find.exec();
    MainWindow::Dialog& d = finddialog();
    QRect pos = find.geometry();
    d.left = pos.left();
    d.top = pos.top();
    if (!res) return;

    QString look = find.getSearchString();
    FindDialog::type searchRange = find.getType();
    if (look.isEmpty()) return;

    Search request(look, searchRange);

    QTextCursor oldCursor = text->textCursor();
    QTextCursor noSelection = oldCursor;
    noSelection.setPosition(oldCursor.position());
    text->setTextCursor(noSelection);

    QColor pen(QColorConstants::Svg::white);
    QColor back(QColorConstants::Svg::blue);
    QColor normalPen(QColorConstants::Svg::black);
    QColor normalBack(QColorConstants::Svg::white);

    bool savedDirty = _dirty;

    for (; ; ) {
        MainWindow::Position found = request.findNext();
        if (found.unset()) {
            Util::OK("Search complete", "Nothing more found");
            break;
        }

        request.current(found);

        QTreeWidgetItem* item = getItemByIndex(found.scene());
        QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
        if (tree->currentItem() != item) tree->setCurrentItem(item);

        cursor.setPosition(found.offset());
        cursor.movePosition(QTextCursor::MoveOperation::NextCharacter, QTextCursor::MoveMode::KeepAnchor, look.size());

        QTextCharFormat format = text->currentCharFormat();
        format.setBackground(back);
        format.setForeground(pen);
        text->setTextCursor(cursor);
        text->ensureCursorVisible();
        text->mergeCurrentCharFormat(format);
        text->setTextCursor(noSelection);
        text->repaint();

        int result = Util::YesNo("Continue Search?", "Find successful");

        format.setBackground(normalBack);
        format.setForeground(normalPen);
        text->setTextCursor(cursor);
        text->mergeCurrentCharFormat(format);
        text->setTextCursor(noSelection);
        text->repaint();

        if (result == QMessageBox::No) break;
    }
    _dirty = savedDirty;
}

void MainWindow::fullJustifyAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->setAlignment(Qt::AlignJustify);
}

void MainWindow::fullScreenAction()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    int currentIdx = current->data(0, Qt::ItemDataRole::UserRole).toInt();
    Scene& currentScene = _scenes[currentIdx];
    if (currentScene._root) return;

    FullScreen fullScreen;

    QTextEdit* mainTextEdit = findChild<QTextEdit*>("textEdit");
    QTextEdit* scrTextEdit = fullScreen.findChild<QTextEdit*>("textEdit");

    scrTextEdit->setDocument(mainTextEdit->document());
    scrTextEdit->setTextCursor(mainTextEdit->textCursor());

    scrTextEdit->setFocus();
    scrTextEdit->ensureCursorVisible();
    fullScreen.showFullScreen();
    fullScreen.exec();

    struct MainWindow::Fullscreen& f = fullscreen();
    f.sizes.clear();
    f.sizes_2.clear();
    f.sizes.append(fullScreen.findChild<QSplitter*>("splitter")->sizes());
    f.sizes_2.append(fullScreen.findChild<QSplitter*>("splitter_2")->sizes());

    mainTextEdit->setTextCursor(scrTextEdit->textCursor());
    mainTextEdit->ensureCursorVisible();
}

void MainWindow::helpAction()
{
    HelpDialog help(_exeDir);
    help.show();
    help.exec();

    MainWindow::Dialog& d = helpdialog();
    QRect pos = help.geometry();
    d.left = pos.left();
    d.top = pos.top();
}

void MainWindow::indentAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor cursor = text->textCursor();
    QTextBlockFormat blk = cursor.blockFormat();
    blk.setIndent(blk.indent() + 1);
    cursor.setBlockFormat(blk);
    text->setTextCursor(cursor);
}

void MainWindow::italicAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor cursor = text->textCursor();
    QTextCharFormat is = cursor.charFormat();
    QTextCharFormat fmt;
    fmt.setFontItalic(!is.fontItalic());
    cursor.mergeCharFormat(fmt);
    text->mergeCurrentCharFormat(fmt);
    _dirty = true;
}

void MainWindow::itemDoubleClickedAction(QTreeWidgetItem* current, int column)
{
    column = (int) (double) column;
    int currentIdx = current->data(0, Qt::ItemDataRole::UserRole).toInt();
    Scene& currentScene = _scenes[currentIdx];

    Scene x(currentScene);
    TagsDialog tags(x);
    tags.show();
    int res = tags.exec();
    MainWindow::Dialog& d = tagdialog();
    QRect pos = tags.geometry();
    d.left = pos.left();
    d.top = pos.top();

    if (res) {
        currentScene = x;
        current->setText(0, x._name);
        _dirty = true;
    }
}

void MainWindow::leftAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->setAlignment(Qt::AlignLeft);
}

void MainWindow::moveDownAction()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    QTreeWidgetItem* parent = current->parent();
    if (parent == nullptr) return;
    int idx = parent->indexOfChild(current);
    if (idx == parent->childCount() - 1) return;
    parent->removeChild(current);
    parent->insertChild(idx + 1, current);
    tree->setCurrentItem(current);
    _dirty = true;
}

void MainWindow::moveInAction()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    QTreeWidgetItem* parent = current->parent();
    if (parent == nullptr) return;
    int idx = parent->indexOfChild(current);
    if (idx == 0) return;
    QTreeWidgetItem* newParent = parent->child(idx - 1);
    parent->removeChild(current);
    newParent->addChild(current);
    newParent->setExpanded(true);
    tree->setCurrentItem(current);
    _dirty = true;
}

void MainWindow::moveOutAction()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    QTreeWidgetItem* parent = current->parent();
    if (parent == nullptr || parent->parent() == nullptr) return;
    QTreeWidgetItem* grandParent = parent->parent();
    int idx = grandParent->indexOfChild(parent);
    parent->removeChild(current);
    grandParent->insertChild(idx + 1, current);
    tree->setCurrentItem(current);
    _dirty = true;
}

void MainWindow::moveUpAction()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    QTreeWidgetItem* parent = current->parent();
    if (parent == nullptr) return;
    int idx = parent->indexOfChild(current);
    if (idx == 0) return;
    parent->removeChild(current);
    parent->insertChild(idx - 1, current);
    tree->setCurrentItem(current);
    _dirty = true;
}

void MainWindow::newAction()
{
    if (_dirty) {
        switch (Util::YesNoCancel("Do you want to save your changes first?", "The current document has been changed!")) {
        case QMessageBox::Yes:    if (save()) break; else return;
        case QMessageBox::Cancel: return;
        default: break;
        }
    }
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    tree->clear();
    QTreeWidgetItem* topLevel = new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr), QStringList("<unnamed>"));
    _scenes.append(Scene("", true));
    topLevel->setData(0, Qt::ItemDataRole::UserRole, QVariant(0));
    tree->insertTopLevelItem(0, topLevel);
    QTreeWidgetItem* scene = new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr), QStringList("<unnamed>"));
    _scenes.append(Scene(""));
    scene->setData(0, Qt::ItemDataRole::UserRole, QVariant(1));
    topLevel->insertChild(0, scene);
}

void MainWindow::newSceneAction()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* parent = tree->currentItem();
    if (parent == nullptr) parent = tree->topLevelItem(0);
    int idx = parent->childCount();
    QTreeWidgetItem* scene = new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr), QStringList("<unnamed>"));
    _scenes.append(Scene(""));
    scene->setData(0, Qt::ItemDataRole::UserRole, QVariant(this->_scenes.size() - 1));
    parent->insertChild(idx, scene);
    _dirty = true;
}

Scene MainWindow::objectToScene(const QJsonObject& obj)
{
    QString name = obj["name"].toString("");
    bool root = obj["root"].toBool(false);
    Scene scene(name, root);
    scene._doc = obj["doc"].toString("");
    scene._wc = obj["wc"].toInt(0);
    const QJsonArray& tags = obj["tags"].toArray();
    for (auto tag: tags) scene._tags.append(tag.toString(""));
    return scene;
}

QTreeWidgetItem* MainWindow::objectToItem(const QJsonObject& obj, int& total)
{
    Scene scene = objectToScene(obj);
    total += scene._wc;
    QTreeWidgetItem* item = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(scene._name.isEmpty() ? "<unnamed>" : scene._name));
    const QJsonArray& children = obj["children"].toArray();
    for (auto child: children) item->addChild(objectToItem(child.toObject(), total));
    item->setData(0, Qt::ItemDataRole::UserRole, QVariant(this->_scenes.size()));
    _scenes.append(scene);
    return item;
}

void MainWindow::openAction()
{
    if (_dirty) {
        switch (Util::YesNoCancel("Do you want to save your changes first?", "The current document has been changed!")) {
        case QMessageBox::Yes:    if (save()) break; else return;
        case QMessageBox::Cancel: return;
        default: break;
        }
    }

    QString filename = QFileDialog::getOpenFileName(this, "Open File", _dir, "Novels (*.novel)");
    if (filename.isEmpty()) return;

    open(filename);
}

static QString slashToBackslash(QString file)
{
    QString result = "";
    int pos;
    while ((pos = file.indexOf("/")) != -1) {
        result += file.left(pos) + "\\";
        file = file.right(file.length() - pos - 1);
    }
    result += file;
    return result;
}

void MainWindow::exportAs(QString type)
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* rootItem = tree->topLevelItem(0);
    int idx = rootItem->data(0, Qt::ItemDataRole::UserRole).toInt();
    if (_scenes[idx]._name == "") if (!saveAs()) return;

    updateSceneWithEdits();

    save();

    QString file = _dir + "/" + _scenes[idx]._name;
    QString input = "\"" + file + ".novel\"";
    QString output = "\"" + file + "." + type.toLower() + "\"";
    QString command = (_exeDir + "/BookSmith" + type + ".exe " + input + " " + output);
    command = slashToBackslash(command);

    if (system(command.toStdString().c_str()) < 0) Util::OK("Unable to export as " + type + ".\nMaybe reinstall BookSmith?");
}

void MainWindow::odfAction()
{
    exportAs("ODT");
}

void MainWindow::outdentAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor cursor = text->textCursor();
    QTextBlockFormat blk = cursor.blockFormat();
    int indent = blk.indent();
    if (indent == 0) return;
    blk.setIndent(indent - 1);
    cursor.setBlockFormat(blk);
    text->setTextCursor(cursor);
}

void MainWindow::pasteAction()
{
    QTextEdit* qtext = findChild<QTextEdit*>("textEdit");
    TextEdit* text = (TextEdit*) qtext;
    if (text->canPaste()) {
        text->paste();
        QTextCursor preCursor = text->textCursor();
        QTextBlockFormat blk = preCursor.blockFormat();
        blk.setTextIndent(20.0);
        blk.setBottomMargin(10.0);
        QTextCursor postCursor = text->textCursor();
        text->setTextCursor(preCursor);
        postCursor.setBlockFormat(blk);
        text->setTextCursor(postCursor);
    }
}

void MainWindow::pdfAction()
{
    exportAs("PDF");
}

void MainWindow::replaceAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor cursor = text->textCursor();
    updateSceneWithEdits();

    ReplaceDialog replace(cursor.hasSelection());
    replace.show();

    int res = replace.exec();
    MainWindow::Dialog& d = replacedialog();
    QRect pos = replace.geometry();
    d.left = pos.left();
    d.top = pos.top();
    if (!res) return;

    QString look = replace.getSearchString();
    QString with = replace.getReplaceString();
    FindDialog::type searchRange = replace.getType();
    if (look.isEmpty()) return;

    Search request(look, searchRange, with);

    QTextCursor oldCursor = text->textCursor();
    QTextCursor noSelection = oldCursor;
    noSelection.setPosition(oldCursor.position());
    text->setTextCursor(noSelection);

    QColor pen(QColorConstants::Svg::white);
    QColor back(QColorConstants::Svg::blue);
    QColor normalPen(QColorConstants::Svg::black);
    QColor normalBack(QColorConstants::Svg::white);

    bool savedDirty = _dirty;
    bool replacingAll = false;
    bool replaced = false;

    for (; ; ) {
        MainWindow::Position found = request.findNext();
        if (found.unset()) {
            Util::OK("Replace complete", "Nothing more found");
            break;
        }

        request.current(found);

        QTreeWidgetItem* item = getItemByIndex(found.scene());
        QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
        if (tree->currentItem() != item) tree->setCurrentItem(item);

        cursor.setPosition(found.offset());
        cursor.movePosition(QTextCursor::MoveOperation::NextCharacter, QTextCursor::MoveMode::KeepAnchor, look.size());

        QTextCharFormat format = text->currentCharFormat();
        format.setBackground(back);
        format.setForeground(pen);
        text->setTextCursor(cursor);
        text->ensureCursorVisible();
        text->mergeCurrentCharFormat(format);
        text->setTextCursor(noSelection);
        text->repaint();

        int result = QMessageBox::Yes;
        if (!replacingAll) result = Util::Question("Replace text?", "Find successful", QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::Cancel);

        format.setBackground(normalBack);
        format.setForeground(normalPen);
        text->setTextCursor(cursor);
        text->mergeCurrentCharFormat(format);

        if (result == QMessageBox::Yes || result == QMessageBox::YesToAll) {
            text->insertPlainText(with);
            found.offset(found.offset() + with.length() - 1);
            request.current(found);
            if (result == QMessageBox::YesToAll) replacingAll = true;
            replaced = true;
        }

        text->setTextCursor(noSelection);
        text->repaint();

        if (result == QMessageBox::No) continue;
        if (result == QMessageBox::Cancel) break;
    }

    _dirty = savedDirty || replaced;
}

void MainWindow::rightAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->setAlignment(Qt::AlignRight);
}

void MainWindow::saveAction()
{
    if (!_dirty) return;
    save();
}

void MainWindow::saveAsAction()
{
    if (saveAs()) save();
}

void MainWindow::sceneShowAction()
{
    static QAction* deleteAction = nullptr;
    static QAction* moveUpAction = nullptr;
    static QAction* moveDownAction = nullptr;
    static QAction* moveInAction = nullptr;
    static QAction* moveOutAction = nullptr;
    static QTreeWidget* tree = nullptr;

    if (!deleteAction) {
        deleteAction = findChild<QAction*>("actionDelete");
        moveUpAction = findChild<QAction*>("actionMove_Up");
        moveDownAction = findChild<QAction*>("actionMove_Down");
        moveInAction = findChild<QAction*>("actionMove_In");
        moveOutAction = findChild<QAction*>("actionMove_Out");
        tree = findChild<QTreeWidget*>("treeWidget");
    }

    QTreeWidgetItem* current = tree->currentItem();
    QTreeWidgetItem* parent = current->parent();
    deleteAction->setEnabled(parent != nullptr);
    moveUpAction->setEnabled(parent != nullptr && parent->indexOfChild(current) != 0);
    moveDownAction->setEnabled(parent != nullptr && parent->indexOfChild(current) != parent->childCount() - 1);
    moveInAction->setEnabled(parent != nullptr && parent->indexOfChild(current) != 0);
    moveOutAction->setEnabled(parent != nullptr && parent->parent() != nullptr);
}

void MainWindow::textAction()
{
    exportAs("TXT");
}

static int countWords(QString x)
{
    std::string y = x.toStdString();
    bool spacing = (y[0] == ' ' || y[0] == '\t' || y[0] == '\r' || y[0] == '\n' || y[0] == '\0');
    int count = spacing ? 0 : 1;
    for (const char* c = y.c_str(); *c; ++c) {
        if (spacing) {
            spacing = (*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n');
            if (!spacing) ++count;
        }
        else spacing = (*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n');
    }
    return count;
}

void MainWindow::textChangedAction()
{
    change();
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    if (current == nullptr) current = tree->topLevelItem(0);
    int currentIdx = current->data(0, Qt::ItemDataRole::UserRole).toInt();
    Scene& scene = _scenes[currentIdx];
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextDocument* doc = text->document();
    QString str = doc->toPlainText();
    int count = countWords(str);
    _totalWc -= scene._wc;
    scene._wc = count;
    _totalWc += scene._wc;

    QString display = QString("%1/%2").arg(scene._wc).arg(_totalWc);
    _wcLabel->setText(display);
}

void MainWindow::underlineAction()
{
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTextCursor cursor = text->textCursor();
    QTextCharFormat is = cursor.charFormat();
    QTextCharFormat fmt;
    fmt.setFontUnderline(!is.fontUnderline());
    cursor.mergeCharFormat(fmt);
    text->mergeCurrentCharFormat(fmt);
    _dirty = true;
}
