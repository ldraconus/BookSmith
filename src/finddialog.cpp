#include "finddialog.h"
#include "mainwindow.h"
#include "ui_finddialog.h"
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QPushButton>

FindDialog::FindDialog(bool selection, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindDialog)
{
    ui->setupUi(this);
    QRadioButton* select = findChild<QRadioButton*>("selectionRadioButton");
    QRadioButton* scene = findChild<QRadioButton*>("sceneRadioButton");
    select->setEnabled(selection);
    if (selection) select->setChecked(true);
    else scene->setChecked(true);
    QDialogButtonBox* box = findChild<QDialogButtonBox*>("buttonBox");
    QPushButton* ok = box->button(QDialogButtonBox::StandardButton::Ok);
    ok->setEnabled(false);

    MainWindow::Dialog& d = MainWindow::getMainWindow()->finddialog();
    QRect pos = geometry();
    int width = pos.width();
    int height = pos.height();
    if (d.top >= 0) {
        pos.setTop(d.top);
        if (d.left >= 0) pos.setLeft(d.left);
        pos.setWidth(width);
        pos.setHeight(height);
        setGeometry(pos);
    }

    connect(findChild<QLineEdit*>("lineEdit"), SIGNAL(textChanged(const QString &)), this, SLOT(changedText(const QString &)));
}

FindDialog::~FindDialog()
{
    delete ui;
}

FindDialog::type FindDialog::getType()
{
    if (findChild<QRadioButton*>("selectionRadioButton")->isChecked()) return FindDialog::Selection;
    if (findChild<QRadioButton*>("sceneRadioButton")->isChecked()) return FindDialog::Scene;
    if (findChild<QRadioButton*>("childrenRadioButton")->isChecked()) return FindDialog::SceneChildren;
    if (findChild<QRadioButton*>("siblingsRadioButton")->isChecked()) return FindDialog::SiblingChildren;
    return FindDialog::All;
}

QString FindDialog::getSearchString()
{
    return findChild<QLineEdit*>("lineEdit")->text();
}

void FindDialog::changedText(const QString &text)
{
    QDialogButtonBox* box = findChild<QDialogButtonBox*>("buttonBox");
    QPushButton* ok = box->button(QDialogButtonBox::StandardButton::Ok);
    ok->setEnabled(!text.isEmpty());
}
