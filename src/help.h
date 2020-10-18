#ifndef HELP_H
#define HELP_H

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
    Ui::HelpDialog *ui;
    QString _dir;
};

#endif // HELP_H
