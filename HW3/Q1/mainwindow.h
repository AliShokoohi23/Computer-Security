#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_importKeyButton_clicked();
    void on_encryptTextButton_clicked();
    void on_encryptFileButton_clicked();
    void on_decryptButton_clicked();
private:
    void initializeLogTable();
    void addLogEntry(const QString &message);

    void updatePublicKeyList();
    void updatePrivateKeyList();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
