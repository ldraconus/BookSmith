#ifndef UTIL_H
#define UTIL_H

#include <QMessageBox>
#include "mainwindow.h"

class Util
{
public:
    Util(MainWindow::Dialog& ok, MainWindow::Dialog& okcancel, MainWindow::Dialog& yesno, MainWindow::Dialog& yesnocancel, MainWindow::Dialog& question);

    static MainWindow::Dialog* _ok;
    static MainWindow::Dialog* _okcancel;
    static MainWindow::Dialog* _yesno;
    static MainWindow::Dialog* _yesnocancel;
    static MainWindow::Dialog* _question;

    static int YesNo(const char* question, const char* title = nullptr);
    static int YesNoCancel(const char* question, const char* title = nullptr);
    static int OK(const char* question, const char* title = nullptr);
    static int OKCancel(const char* question, const char* title = nullptr);
    static int Question(const char* question, const char* title = nullptr, QFlags<QMessageBox::StandardButton> buttons = QMessageBox::Ok | QMessageBox::Cancel);

    static int YesNo(const QString& msg, const QString& title) { return YesNo(msg.toStdString().c_str(), title == "" ? nullptr : msg.toStdString().c_str()); }
    static int YesNoCancel(const QString& msg, const QString& title) { return YesNoCancel(msg.toStdString().c_str(), title == "" ? nullptr : msg.toStdString().c_str()); }
    static int OK(const QString& msg, const QString& title) { return OK(msg.toStdString().c_str(), title == "" ? nullptr : msg.toStdString().c_str()); }
    static int OKCancel(const QString& msg, const QString& title) { return OKCancel(msg.toStdString().c_str(), title == "" ? nullptr : msg.toStdString().c_str()); }
    static int Question(const QString& msg, const QString& title, QFlags<QMessageBox::StandardButton> buttons) {  return Question(msg.toStdString().c_str(), title == "" ? nullptr : msg.toStdString().c_str(),  buttons); }
};

#endif // UTIL_H
