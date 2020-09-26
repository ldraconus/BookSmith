#include "replacedialog.h"
#include "ui_replacedialog.h"
#include "mainwindow.h"
#include <QPushButton>

ReplaceDialog::ReplaceDialog(bool selection, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReplaceDialog)
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

    MainWindow::Dialog& d = MainWindow::getMainWindow()->replacedialog();
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
    connect(box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

FindDialog::type ReplaceDialog::getType()
{
    if (findChild<QRadioButton*>("selectionRadioButton")->isChecked()) return FindDialog::Selection;
    if (findChild<QRadioButton*>("sceneRadioButton")->isChecked()) return FindDialog::Scene;
    if (findChild<QRadioButton*>("childrenRadioButton")->isChecked()) return FindDialog::SceneChildren;
    if (findChild<QRadioButton*>("siblingRadioButton")->isChecked()) return FindDialog::SiblingChildren;
    return FindDialog::All;
}

QString ReplaceDialog::getSearchString()
{
    return findChild<QLineEdit*>("lineEdit")->text();
}

QString ReplaceDialog::getReplaceString()
{
    return findChild<QLineEdit*>("lineEdit_2")->text();
}

void ReplaceDialog::changedText(const QString &text)
{
    QDialogButtonBox* box = findChild<QDialogButtonBox*>("buttonBox");
    QPushButton* ok = box->button(QDialogButtonBox::StandardButton::Ok);
    ok->setEnabled(!text.isEmpty());
}
