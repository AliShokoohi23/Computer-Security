#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts>
#include "cryptotest.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartTest();
    void onExportCSV();

private:
    Ui::MainWindow *ui;
    CryptoTest *cryptoTest;

    QVector<double> m_aesResults;
    QVector<double> m_rsaResults;
    void createBoxPlot(const QVector<double>& aesData, const QVector<double>& rsaData);
};

#endif // MAINWINDOW_H
