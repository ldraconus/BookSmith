#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QString dir, QWidget *parent = nullptr);
    ~HelpDialog();

private:
    QString _dir;

private:
    Ui::HelpDialog *ui;
};

#endif // HELPDIALOG_H
