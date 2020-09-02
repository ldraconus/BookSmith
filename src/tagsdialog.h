#ifndef TAGSDIALOG_H
#define TAGSDIALOG_H

#include <QDialog>
#include <QPushButton>

#include "mainwindow.h"

namespace Ui {
class TagsDialog;
}

class TagsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagsDialog(Scene& s, QWidget *parent = 0);
    ~TagsDialog();

    Scene& scene() { return *_scene; }
    void setScene(Scene& s) { _scene = &s; }

    void addTagButton(int i, const QString& tags);
    void addTags(const QList<QString>& tags);

public slots:
    void buttonClickedAction();
    void returnPressedAction();
    void textChangedAction(const QString& text);

protected:
    virtual void keyPressEvent(QKeyEvent* e);

private:
    Ui::TagsDialog *ui;

    Scene* _scene;
    QList<QPushButton*> _buttons;
};

#endif // TAGSDIALOG_H
