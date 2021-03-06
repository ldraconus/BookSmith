#include "help.h"
#include "ui_help.h"

#include "mainwindow.h"
#include "util.h"

#include <QFile>
#include <QTextEdit>

HelpDialog::HelpDialog(QString dir, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);

    _dir = dir;
    setWindowFlags(Qt::Dialog);
    MainWindow::Dialog& d = MainWindow::getMainWindow()->helpdialog();
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

    QTextEdit* text = findChild<QTextEdit*>("textEdit");
    QFile file(_dir + "/docs/help.html");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    auto contents = file.readAll();
    std::string html = contents.toStdString();
    text->setHtml(QString(html.c_str()));
    file.close();
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
