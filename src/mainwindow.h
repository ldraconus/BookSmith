#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "finddialog.h"

#include <QMainWindow>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QTextCursor>
#include <QTextDocument>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QKeyEvent>
#include <QToolBar>

class Scene {
private:
public:
    QString _name;
    bool _root;
    QString _doc;
    QList<QString> _tags;
    int _wc;

    Scene(const QString& n, bool r = false): _name(n), _root(r), _doc(""), _wc(0) { }
    Scene(const Scene& s): _name(s._name), _root(s._root), _doc(s._doc), _tags(s._tags), _wc(s._wc) { }

    Scene& operator=(const Scene& x) { if (this != &x) { _name = x._name; _root = x._root; _doc = x._doc; _tags = x._tags; _wc = x._wc; } return *this; }
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct Fullscreen {
        QList<int> sizes;
        QList<int> sizes_2;
    };

    struct Dialog {
        int top;
        int left;
    };

    explicit MainWindow(const QString& file, QWidget* parent = 0);
    ~MainWindow();

    void change() { _dirty = true; }
    QList<int> getChildren(int idx);
    QTreeWidgetItem* getItemByIndex(int idx);

    bool save();
    bool saveAs();
    bool open(const QString& filename);

    int getParent(int idx);

    void closeEvent(QCloseEvent* event);
    void createToolBarItem(QToolBar* tb, const QString& icon, const QString& name, const QString& tip, const char* signal, const char* slot);
    void resizeEvent(QResizeEvent* event);

    struct Fullscreen& fullscreen() { return _fullscreen; }
    struct Dialog& finddialog()     { return _finddialog; }
    struct Dialog& replacedialog()  { return _replacedialog; }
    struct Dialog& tagdialog()      { return _tagdialog; }
    struct Dialog& helpdialog()     { return _helpdialog; }
    struct Dialog& ok()             { return _ok; }
    struct Dialog& okcancel()       { return _okcancel; }
    struct Dialog& yesno()          { return _yesno; }
    struct Dialog& yesnocancel()    { return _yesnocancel; }
    struct Dialog& question()       { return _question; }
    struct Dialog& statement()      { return _statement; }

    static MainWindow* getMainWindow() { return _mainWindow; }

    class Position {
    private:
        int _scene;
        int _offset;

    public:
        Position(int s = -1, int o = -1): _scene(s), _offset(o) { }
        Position(const Position& p): _scene(p._scene), _offset(p._offset) { }

        Position& operator=(const Position& p) { if (this != &p) { _scene = p._scene; _offset = p._offset; } return *this; }

        bool unset() { return _scene == -1 && _offset == -1; }

        int scene(int s = -1) { if (s >= 0) _scene = s; return _scene; }
        int offset(int o = -1) { if (o >= 0) _offset = o; return _offset; }

        void nil() { _scene = _offset = -1; }
    };

    class Search {
    private:
        QList<int> _stack;
        QString _look;
        QString _with;
        FindDialog::type _range;
        bool _wrapped;
        Position _current;
        Position _start;
        Position _stop;

        void init(FindDialog::type r);

    public:
        Search(QString& l, FindDialog::type r);
        Search(QString& l, FindDialog::type r, QString& w);

        Position findNext();
        Position findNextChild(Position current);

        void current(Position& f) { _current = f; }
    };

private:
    Ui::MainWindow* ui;

    QSize _diff;
    QList<Scene> _scenes;
    QLabel* _wcLabel;
    QString _dir;
    QString _exeDir;
    int _dirty;
    int _totalWc;
    Fullscreen _fullscreen;
    Dialog _finddialog = { -1, -1 };
    Dialog _replacedialog = { -1, -1 };
    Dialog _tagdialog = { -1, -1 };
    Dialog _helpdialog = { -1, -1 };
    Dialog _ok = { -1, -1 };
    Dialog _okcancel = { -1, -1 };
    Dialog _yesno = { -1, -1 };
    Dialog _yesnocancel = { -1, -1 };
    Dialog _question = { -1, -1 };
    Dialog _statement = { -1, -1 };

    bool checkClose();
    void exportAs(QString type);
    void updateSceneWithEdits();
    QJsonObject sceneToObject(const Scene& scene);
    QJsonObject itemToObject(const QTreeWidgetItem* scene);
    Scene objectToScene(const QJsonObject& obj);
    QTreeWidgetItem* objectToItem(const QJsonObject &item, int& total);

    static MainWindow* _mainWindow;

    virtual void keyReleaseEvent(QKeyEvent *e);

public slots:
    void aboutAction();
    void boldAction();
    void centerAction();
    void checkAction();
    void copyAction();
    void currentItemChangedAction(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void cutAction();
    void fullScreenAction();
    void itemDoubleClickedAction(QTreeWidgetItem* current, int column);
    void deleteAction();
    void editShowAction();
    void epubAction();
    void fileShowAction();
    void findAction();
    void fullJustifyAction();
    void helpAction();
    void indentAction();
    void italicAction();
    void leftAction();
    void moveDownAction();
    void moveInAction();
    void moveOutAction();
    void moveUpAction();
    void newSceneAction();
    void newAction();
    void odfAction();
    void openAction();
    void outdentAction();
    void pasteAction();
    void pdfAction();
    void replaceAction();
    void rightAction();
    void saveAction();
    void saveAsAction();
    void sceneShowAction();
    void textAction();
    void textChangedAction();
    void underlineAction();
};

#endif // MAINWINDOW_H
