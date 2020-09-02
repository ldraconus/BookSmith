#include "util.h"

Util::Util()
{

}

int Util::YesNo(const char* msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    return msgBox.exec();
}

int Util::YesNoCancel(const char* msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("Are you really sure?");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    return msgBox.exec();
}

int Util::OK(const char* msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Something has happened.");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    return msgBox.exec();
}

int Util::OKCancel(const char* msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("Something bad is about to happened.");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    return msgBox.exec();
}

int Util::YesNo(const QString& msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    return msgBox.exec();
}

int Util::YesNoCancel(const QString& msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("Are you really sure?");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    return msgBox.exec();
}

int Util::OK(const QString& msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Something has happened.");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    return msgBox.exec();
}

int Util::OKCancel(const QString& msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("Something bad is about to happened.");
    msgBox.setInformativeText(msg);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    return msgBox.exec();
}
