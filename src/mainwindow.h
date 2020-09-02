#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QTextCursor>
#include <QTextDocument>
#include <QTreeWidgetItem>
#include <QLabel>

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
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    bool save();
    bool saveAs();
    void change() { _dirty = true; }

    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent* event);

    static MainWindow* getMainWindow() { return _mainWindow; }

private:
    Ui::MainWindow* ui;

    QSize _diff;
    QList<Scene> _scenes;
    QLabel* _wcLabel;
    QString _dir;
    int _dirty;
    int _totalWc;

    bool checkClose();
    QJsonObject sceneToObject(const Scene& scene);
    QJsonObject itemToObject(const QTreeWidgetItem* scene);
    Scene objectToScene(const QJsonObject& obj);
    QTreeWidgetItem* objectToItem(const QJsonObject &item, int& total);

    static MainWindow* _mainWindow;

public slots:
    void boldAction();
    void centerAction();
    void checkAction();
    void copyAction();
    void currentItemChangedAction(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void cutAction();
    void fullScreenAction();
    void itemDoubleClickedAction(QTreeWidgetItem* current, int column);
    void deleteAction();
    void fullJustifyAction();
    void indentAction();
    void italicAction();
    void leftAction();
    void newAction();
    void outdentAction();
    void moveDownAction();
    void moveInAction();
    void moveOutAction();
    void moveUpAction();
    void newSceneAction();
    void openAction();
    void pasteAction();
    void rightAction();
    void saveAction();
    void saveAsAction();
    void textChangedAction();
    void underlineAction();
};

#endif // MAINWINDOW_H
