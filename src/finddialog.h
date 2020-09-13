#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

namespace Ui {
class FindDialog;
}

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(bool selection, QWidget *parent = nullptr);
    ~FindDialog();

    enum type { Selection, Scene, SceneChildren, SiblingChildren, All };

    type getType();
    QString getSearchString();

public slots:
    void changedText(const QString &text);

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H
