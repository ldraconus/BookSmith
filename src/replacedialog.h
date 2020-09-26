#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include "finddialog.h"
#include <QDialog>

namespace Ui {
    class ReplaceDialog;
}

class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(bool selection, QWidget *parent = nullptr);
    ~ReplaceDialog();

    FindDialog::type getType();
    QString getSearchString();
    QString getReplaceString();

public slots:
    void changedText(const QString &text);

private:
    Ui::ReplaceDialog *ui;
};

#endif // REPLACEDIALOG_H
