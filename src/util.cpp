#include "util.h"
#include "mainwindow.h"

MainWindow::Dialog* Util::_ok;
MainWindow::Dialog* Util::_okcancel;
MainWindow::Dialog* Util::_yesno;
MainWindow::Dialog* Util::_yesnocancel;
MainWindow::Dialog* Util::_question;
MainWindow::Dialog* Util::_statement;

Util::Util(MainWindow::Dialog& ok, MainWindow::Dialog& okcancel, MainWindow::Dialog& yesno, MainWindow::Dialog& yesnocancel, MainWindow::Dialog& question, MainWindow::Dialog& statement)
{
    _ok = &ok;
    _okcancel = &okcancel;
    _yesno = &yesno;
    _yesnocancel = &yesnocancel;
    _question = &question;
    _statement = &statement;
}

static int showMsgBox(QMessageBox& msgBox, MainWindow::Dialog* save)
{
    QRect pos = msgBox.geometry();
    int width = pos.width();
    int height = pos.height();
    if (save->top >= 0) {
        pos.setTop(save->top);
        if (save->left >= 0) pos.setLeft(save->left);
        pos.setWidth(width);
        pos.setHeight(height);
        msgBox.setGeometry(pos);
    }
    int res = msgBox.exec();
    pos = msgBox.geometry();
    save->left = pos.left();
    save->top = pos.top();
    return res;
}

int Util::YesNo(const char* msg, const char* title)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(title ? title : "Are you sure?");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    return showMsgBox(msgBox, _yesno);
}

int Util::YesNoCancel(const char* msg, const char* title)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(title ? title : "Are you really sure?");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    return showMsgBox(msgBox, _yesnocancel);
}

int Util::OK(const char* msg, const char* title)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(title ? title : "Something has happened.");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    return showMsgBox(msgBox, _ok);
}

int Util::OKCancel(const char* msg, const char* title)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(title ? title : "Something bad is about to happened.");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    return showMsgBox(msgBox, _okcancel);
}

int Util::Question(const char* msg, const char* title, QFlags<QMessageBox::StandardButton> buttons)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(title ? title : "Are you sure?");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(buttons);
    return showMsgBox(msgBox, _question);
}

int Util::Statement(const char* msg)
{
    QMessageBox msgBox;
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Ok);
    return showMsgBox(msgBox, _statement);
}
