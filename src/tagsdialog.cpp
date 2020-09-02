#include "tagsdialog.h"
#include "ui_tagsdialog.h"

#include <QPushButton>

TagsDialog::TagsDialog(Scene& s, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TagsDialog),
    _scene(&s)
{
    ui->setupUi(this);

    connect(findChild<QLineEdit*>("lineEdit_2"), SIGNAL(returnPressed()),             this, SLOT(returnPressedAction()));
    connect(findChild<QLineEdit*>("lineEdit"),   SIGNAL(textChanged(const QString&)), this, SLOT(textChangedAction(const QString&)));

    findChild<QLineEdit*>("lineEdit")->setText(_scene->_name);
    addTags(_scene->_tags);
}

TagsDialog::~TagsDialog()
{
    delete ui;
}

void TagsDialog::addTagButton(int i, const QString& tag)
{
    QPushButton *button = new QPushButton(tag, this);
    button->setCheckable(true);

    QFrame* edit = findChild<QFrame*>("frame");
    int x = (i % 3) * button->geometry().width() + edit->geometry().x() + (i % 3 + 1) * 6;
    int y = (i / 3) * button->geometry().height() + edit->geometry().y() + edit->geometry().height() + (i / 3 + 1) * 6;
    QRect g = button->geometry();
    g.setX(x);
    g.setY(y);
    button->setGeometry(g.x(), g.y(), g.x() + g.width(), g.y() + g.height());

    connect(button, SIGNAL(clicked()), this, SLOT(buttonClickedAction()));
    _buttons.append(button);
    button->setEnabled(true);
    button->setVisible(true);
    repaint();
}

void TagsDialog::addTags(const QList<QString>& tags)
{
    int i = 0;
    for (auto x: tags)
        addTagButton(i++, x);
}

void TagsDialog::buttonClickedAction()
{
    for (auto x: _buttons) {
        if (x->isChecked()) {
            QString tag = x->text();
            _scene->_tags.removeOne(tag);
        }
    }

    for (auto x: _buttons) delete x;
    _buttons.clear();

    addTags(_scene->_tags);
}

void TagsDialog::returnPressedAction()
{
    QLineEdit* edit = findChild<QLineEdit*>("lineEdit_2");
    QString text = edit->text();
    if (text == "") return;

    edit->setText("");

    if (_scene->_tags.indexOf(text) != -1) return;

    _scene->_tags.append(text);
    for (auto x: _buttons) delete x;
    _buttons.clear();

    addTags(_scene->_tags);
}

void TagsDialog::textChangedAction(const QString& text)
{
    _scene->_name = text;
}

void TagsDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return) return;
    QDialog::keyPressEvent(e);
}

