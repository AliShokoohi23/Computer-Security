#ifndef KEYDIALOG_H
#define KEYDIALOG_H

#include <QDialog>

namespace Ui {
class KeyDialog;
}

class KeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyDialog(QWidget *parent = nullptr);
    ~KeyDialog();

public slots:
    void on_generateKeyButton_clicked();
    void on_continueButton_clicked();

private:
    Ui::KeyDialog *ui;
};

#endif // KEYDIALOG_H
