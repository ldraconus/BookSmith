#include "mainwindow.h"
#include "fullscreen.h"
#include "tagsdialog.h"
#include "ui_mainwindow.h"
#include "util.h"

#include <QDir>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MainWindow* MainWindow::_mainWindow = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _mainWindow = this;
    _dirty = false;

    connect(findChild<QAction*>("actionExit"),        SIGNAL(triggered()),                                           this, SLOT(checkAction()));
    connect(findChild<QAction*>("actionFull_Screen"), SIGNAL(triggered()),                                           this, SLOT(fullScreenAction()));
    connect(findChild<QAction*>("actionNew"),         SIGNAL(triggered()),                                           this, SLOT(newAction()));
    connect(findChild<QAction*>("actionOpen"),        SIGNAL(triggered()),                                           this, SLOT(openAction()));
    connect(findChild<QAction*>("actionSave"),        SIGNAL(triggered()),                                           this, SLOT(saveAction()));
    connect(findChild<QAction*>("actionSave_As"),     SIGNAL(triggered()),                                           this, SLOT(saveAsAction()));
    connect(findChild<QAction*>("actionNewScene"),    SIGNAL(triggered()),                                           this, SLOT(newSceneAction()));
    connect(findChild<QAction*>("actionDelete"),      SIGNAL(triggered()),                                           this, SLOT(deleteAction()));
    connect(findChild<QAction*>("actionMove_In"),     SIGNAL(triggered()),                                           this, SLOT(moveInAction()));
    connect(findChild<QAction*>("actionMove_Out"),    SIGNAL(triggered()),                                           this, SLOT(moveOutAction()));
    connect(findChild<QAction*>("actionMove_Up"),     SIGNAL(triggered()),                                           this, SLOT(moveUpAction()));
    connect(findChild<QAction*>("actionMove_Down"),   SIGNAL(triggered()),                                           this, SLOT(moveDownAction()));
    connect(findChild<QAction*>("actionCut"),         SIGNAL(triggered()),                                           this, SLOT(cutAction()));
    connect(findChild<QAction*>("actionCopy"),        SIGNAL(triggered()),                                           this, SLOT(copyAction()));
    connect(findChild<QAction*>("actionPaste"),       SIGNAL(triggered()),                                           this, SLOT(pasteAction()));
    connect(findChild<QAction*>("actionBold"),        SIGNAL(triggered()),                                           this, SLOT(boldAction()));
    connect(findChild<QAction*>("actionItalic"),      SIGNAL(triggered()),                                           this, SLOT(italicAction()));
    connect(findChild<QAction*>("actionUnderline"),   SIGNAL(triggered()),                                           this, SLOT(underlineAction()));
    connect(findChild<QAction*>("actionLeft"),        SIGNAL(triggered()),                                           this, SLOT(leftAction()));
    connect(findChild<QAction*>("actionCenter"),      SIGNAL(triggered()),                                           this, SLOT(centerAction()));
    connect(findChild<QAction*>("actionRight"),       SIGNAL(triggered()),                                           this, SLOT(rightAction()));
    connect(findChild<QAction*>("actionJustify"),     SIGNAL(triggered()),                                           this, SLOT(fullJustifyAction()));
    connect(findChild<QAction*>("actionIndent"),      SIGNAL(triggered()),                                           this, SLOT(indentAction()));
    connect(findChild<QAction*>("actionOutdent"),     SIGNAL(triggered()),                                           this, SLOT(outdentAction()));
    connect(findChild<QTextEdit*>("textEdit"),        SIGNAL(textChanged()),                                         this, SLOT(textChangedAction()));
    connect(findChild<QTreeWidget*>("treeWidget"),    SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChangedAction(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(findChild<QTreeWidget*>("treeWidget"),    SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),               this, SLOT(itemDoubleClickedAction(QTreeWidgetItem*,int)));

    connect(findChild<QMenu*>("menuFile"),  SIGNAL(aboutToShow()), this, SLOT(fileShowAction()));
    connect(findChild<QMenu*>("menuScene"), SIGNAL(aboutToShow()), this, SLOT(sceneShowAction()));
    connect(findChild<QMenu*>("menuEdit"),  SIGNAL(aboutToShow()), this, SLOT(editShowAction()));

    QApplication::setWindowIcon(QIcon("./gfx/BookSmith.ico"));

    // [TODO] restore previous sizes
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

    // [TODO] read args and open book if one is supplied
    newAction();
}

MainWindow::~MainWindow()
{
    delete ui;
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

bool MainWindow::save()
{
    if (_scenes[0]._name == "<unnamed>") if (!saveAs()) return false;

    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem* current = tree->currentItem();
    int currentIdx = -1;
    if (current) currentIdx = current->data(0, Qt::ItemDataRole::UserRole).toInt();
        if (currentIdx < _scenes.size()) {
            Scene& currentScene = _scenes[currentIdx];
            if (current && !currentScene._root) currentScene._doc = text->toHtml();
        }

    QJsonObject root;
    QTreeWidgetItem* rootItem = tree->topLevelItem(0);
    root.insert("root", itemToObject(rootItem));
    QJsonObject top;
    top.insert("document", root);
    QJsonDocument json;
    json.setObject(top);
    QFile file(_dir + "/" + _scenes[0]._name + ".novel");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
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
    int sep = filename.lastIndexOf(QDir::separator());
    if (sep != -1) {
        _dir = filename.left(sep + 1);
        filename = filename.mid(sep + 1);
    }
    _scenes[0]._name = filename;
    return true;
}

bool MainWindow::checkClose()
{
    if (_dirty) {
        switch (Util::YesNoCancel("Do you want to save the\nDocument first?")) {
        case QMessageBox::Yes: if (save()) break; else return false;
        case QMessageBox::No:  break;
        default:               return false;
        }
    }
    else {
        switch (Util::YesNo("Are you sure you want to quit?")) {
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
    switch (Util::YesNo("This action cannot be undone! Abort it?")) {
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

void MainWindow::fileShowAction()
{
    QAction* saveAction = findChild<QAction*>("actionSave");
    saveAction->setEnabled(_dirty);
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

    mainTextEdit->setTextCursor(scrTextEdit->textCursor());
    mainTextEdit->ensureCursorVisible();
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
    if (tags.exec()) {
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
        switch (Util::YesNoCancel("You haven't saved your latest changes!\nDo you want to save them first?")) {
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
    QTreeWidgetItem* item = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(scene._name));
    const QJsonArray& children = obj["children"].toArray();
    for (auto child: children) item->addChild(objectToItem(child.toObject(), total));
    item->setData(0, Qt::ItemDataRole::UserRole, QVariant(this->_scenes.size()));
    _scenes.append(scene);
    return item;
}

void MainWindow::openAction()
{
    if (_dirty) {
        switch (Util::YesNoCancel("You haven't saved your latest changes!\nDo you want to save them first?")) {
        case QMessageBox::Yes:    if (save()) break; else return;
        case QMessageBox::Cancel: return;
        default: break;
        }
    }

    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidget");

    QString filename = QFileDialog::getOpenFileName(this, "Open File", _dir, "Novels (*.novel)");
    if (filename.isEmpty()) return;
    int ext = filename.lastIndexOf(".novel");
    if (ext != -1) filename = filename.left(ext);
    int sep = filename.lastIndexOf("/");
    if (sep != -1) {
        _dir = filename.left(sep + 1);
        filename = filename.mid(sep + 1);
    }

    QFile file(_dir + "/" + filename + ".novel");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) return;
    QByteArray data(file.readAll());
    QString jsonStr(data);
    file.close();
    QJsonDocument json = QJsonDocument::fromJson(jsonStr.toUtf8());

    // scan the json structure in for valid novel
    const QJsonObject& top = json.object();
    if (!top.contains("document")) return;

    _dirty = false;
    const QJsonObject& doc = top["document"].toObject();
    const QJsonObject& root = doc["root"].toObject();
    tree->clear();
    _scenes.clear();
    tree->addTopLevelItem(objectToItem(root, _totalWc));

    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    text->clear();
    text->setEnabled(false);
    _dirty = false;
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
    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    if (text->canPaste()) {
        text->paste();
        QTextCursor postCursor = text->textCursor();
        QTextBlockFormat blk = postCursor.blockFormat();
        blk.setTextIndent(20.0);
        blk.setBottomMargin(10.0);
        postCursor.setBlockFormat(blk);
        text->setTextCursor(postCursor);
    }
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
    saveAs();
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
