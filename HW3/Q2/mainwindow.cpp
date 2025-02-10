#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , cryptoTest(new CryptoTest(this))
{
    ui->setupUi(this);

    // Connect signals/slots
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartTest);
    connect(ui->exportButton, &QPushButton::clicked, this, &MainWindow::onExportCSV);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// SLOT: Start Test
void MainWindow::onStartTest()
{
    ui->startButton->setEnabled(false);
    ui->statusbar->showMessage("Generating random messages...");

    // Example: generate 1000 random messages of 128 bytes each
    auto messages = cryptoTest->generateRandomMessages(1000, 128);

    ui->statusbar->showMessage("Testing AES encryption...");
    m_aesResults = cryptoTest->testAES(messages);

    ui->statusbar->showMessage("Testing RSA encryption...");
    m_rsaResults = cryptoTest->testRSA(messages);

    createBoxPlot(m_aesResults, m_rsaResults);

    ui->statusbar->showMessage("Test completed!");
    ui->startButton->setEnabled(true);
}

// SLOT: Export to CSV
void MainWindow::onExportCSV()
{
    if (m_aesResults.isEmpty() || m_rsaResults.isEmpty()) {
        QMessageBox::warning(this, "No data", "No encryption results available to export.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export CSV"),
        "",
        tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty()) {
        return; // User canceled
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "AES-128 (ms),RSA-3072 (ms)\n";

    // For each index up to the smaller size of the two vectors:
    int rows = qMin(m_aesResults.size(), m_rsaResults.size());
    for (int i = 0; i < rows; i++) {
        out << QString::number(m_aesResults[i], 'f', 6) << ","
            << QString::number(m_rsaResults[i], 'f', 6) << "\n";
    }

    file.close();
    QMessageBox::information(this, "Export Done", "Results exported successfully!");
}

void MainWindow::createBoxPlot(const QVector<double> &aesData, const QVector<double> &rsaData)
{
    // Clear any existing widgets in the chart layout
    QLayout* layout = ui->chartWidget->layout();
    while (layout->count() > 0) {
        QLayoutItem* item = layout->takeAt(0);
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }

    // Sort copies of data
    QVector<double> aesDataCopy = aesData;
    QVector<double> rsaDataCopy = rsaData;
    std::sort(aesDataCopy.begin(), aesDataCopy.end());
    std::sort(rsaDataCopy.begin(), rsaDataCopy.end());

    // Create QBoxSets
    QBoxSet *aesBox = new QBoxSet("AES-128");
    QBoxSet *rsaBox = new QBoxSet("RSA-3072");

    // Fill AES box stats
    aesBox->setValue(QBoxSet::LowerExtreme, aesDataCopy.first());
    aesBox->setValue(QBoxSet::UpperExtreme, aesDataCopy.last());
    aesBox->setValue(QBoxSet::Median, aesDataCopy[aesDataCopy.size() / 2]);
    aesBox->setValue(QBoxSet::LowerQuartile, aesDataCopy[aesDataCopy.size() / 4]);
    aesBox->setValue(QBoxSet::UpperQuartile, aesDataCopy[3 * aesDataCopy.size() / 4]);

    // Fill RSA box stats
    rsaBox->setValue(QBoxSet::LowerExtreme, rsaDataCopy.first());
    rsaBox->setValue(QBoxSet::UpperExtreme, rsaDataCopy.last());
    rsaBox->setValue(QBoxSet::Median, rsaDataCopy[rsaDataCopy.size() / 2]);
    rsaBox->setValue(QBoxSet::LowerQuartile, rsaDataCopy[rsaDataCopy.size() / 4]);
    rsaBox->setValue(QBoxSet::UpperQuartile, rsaDataCopy[3 * rsaDataCopy.size() / 4]);

    // Create the series
    QBoxPlotSeries *series = new QBoxPlotSeries();
    series->append(aesBox);
    series->append(rsaBox);

    // Create the chart
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Encryption Time Comparison (ms)");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignTop);

    // Y-axis
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Time (ms)");
    axisY->setLabelFormat("%.3f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // X-axis
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append({"AES-128", "RSA-3072"});
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // ChartView
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(600, 400);

    // Add chartView to the layout
    ui->chartLayoutContainer->addWidget(chartView);
}
