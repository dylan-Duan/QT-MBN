#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtGlobal>
#include <kiss_fft.h>      // Make sure to add INCLUDEPATH and LIBS in .pro
#include <QTemporaryDir>  // For creating temporary directory


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow())
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::analyzeSignalFeatures(int index)
{
    if (index < 0 || index >= mbnMatrix.size()) return;
    const DoubleVector &mbn = mbnMatrix[index];
    int N = mbn.size();
    if (N == 0) return;

    // 1. Mean & RMS
    double sumAbs = 0.0, sumSq = 0.0;
    for (double v : mbn) {
        sumAbs += std::abs(v);
        sumSq  += v * v;
    }
    double meanVal = sumAbs / N;
    double rmsVal  = std::sqrt(sumSq / N);

    // 2. Ringing count: convert to QList<double>
    QList<double> mbnList = mbn.toList();
    int ringCount = countRingingByPeaks(mbnList, 0.02);

    // 3. Output Mean/RMS/Ringing count
    log(QString("=== Signal %1 Features ===").arg(index + 1));
    log(QString("Mean_Value[%1] = %2").arg(index).arg(meanVal, 0, 'f', 15));
    log(QString("RMS_Value[%1]  = %2").arg(index).arg(rmsVal,  0, 'f', 7));
    log(QString("Number of ringing = %1").arg(ringCount));

    // 4. Envelope peak features (FWHM, ratio, etc.)
    if (index < envelopeMatrix.size()) {
        QList<double> envList = envelopeMatrix[index].toList();
        auto peaks = findPeaksWithWidth(envList, 100000.0, 0.2);
        for (int j = 0; j < peaks.size(); ++j) {
            const auto &p = peaks[j];
            log(QString("Peak %1: amplitude=%2, FWHM=%3 s, ratio=%4")
                    .arg(j + 1)
                    .arg(p.amplitude, 0, 'f', 3)
                    .arg(p.fwhm,      0, 'f', 6)
                    .arg(p.ratio,     0, 'f', 3));
        }
    }
}

void MainWindow::log(const QString &s)
{
    ui->textLog->append(s);
}

void MainWindow::on_btnBrowse_clicked()
{
    // Only support selecting a single .db file
    QString file = QFileDialog::getOpenFileName(this, "Select a single .db file", QString(), "SQLite DB (*.db)");
    if (file.isEmpty()) return;
    ui->editDir->setText(file);
}

void MainWindow::on_btnLoad_clicked()
{
    QString path = ui->editDir->text();
    if (path.isEmpty()) {
        log("File path is empty");
        return;
    }

    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        log("Specified path is not a valid file");
        return;
    }

    // Copy to temporary directory, then use loadAllDbFiles(directory)
    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        log("Failed to create temporary directory");
        return;
    }

    const QString dstPath = tmpDir.path() + QLatin1Char('/') + fi.fileName();
    if (QFile::exists(dstPath)) QFile::remove(dstPath);
    if (!QFile::copy(path, dstPath)) {
        log(QString("Failed to copy file to temporary directory: %1").arg(path));
        return;
    }

    // Now only this .db file exists in the temporary dir
    auto allData = loadAllDbFiles(tmpDir.path());
    log(QString("Actually loaded %1 .db file(s)").arg(allData.size()));
    if (allData.isEmpty()) {
        log("No data loaded: file cannot be opened or contains no data");
        return;
    }

    mbnMatrix      = processAllMBN(allData);
    envelopeMatrix = extractEnvelopes(mbnMatrix);
    log(QString("Processed %1 valid MBN signals").arg(mbnMatrix.size()));

    if (!mbnMatrix.isEmpty()) {
        currentIndex = 0;
        plotTimeDomain(currentIndex);
        analyzeSignalFeatures(currentIndex);
    }
}

void MainWindow::on_btnPlotTime_clicked()
{
    if (mbnMatrix.isEmpty()) {
        log("No data detected");
        return;
    }
    plotTimeDomain(currentIndex);
    analyzeSignalFeatures(currentIndex);
}

void MainWindow::on_btnPlotFreq_clicked()
{
    if (mbnMatrix.isEmpty()) {
        log("No data detected");
        return;
    }
    plotFrequencySpectrum(currentIndex);
}

void MainWindow::on_btnPlotEnv_clicked()
{
    if (envelopeMatrix.isEmpty()) {
        log("No data detected");
        return;
    }
    plotEnvelope(currentIndex);
    analyzeSignalFeatures(currentIndex);
}

void MainWindow::plotTimeDomain(int /*index*/)
{
    QChart *chart = new QChart();

    for (const DoubleVector &mbn : mbnMatrix) {
        QLineSeries *series = new QLineSeries();
        for (int i = 0; i < mbn.size(); ++i) {
            series->append(i, mbn[i]);
        }
        chart->addSeries(series);
    }

    chart->createDefaultAxes();
    chart->setTitle("MBN Curves");

    chart->axisX()->setTitleText("Time (sample index)");
    chart->axisY()->setTitleText("MBN Amplitude");

    ui->widget->setChart(chart);
}

void MainWindow::plotFrequencySpectrum(int index)
{
    if (index < 0 || index >= mbnMatrix.size())
        return;

    QChart *chart = new QChart();
    chart->setTitle("Frequency Spectrum");

    const int fs = 100000; // Sampling rate
    const DoubleVector &mbn = mbnMatrix[index];
    int N = mbn.size();
    if (N == 0) {
        ui->widget->setChart(chart);
        return;
    }

    int Nfft = 1 << static_cast<int>(std::ceil(std::log2(N)));

    kiss_fft_cfg cfg = kiss_fft_alloc(Nfft, 0, nullptr, nullptr);
    std::vector<kiss_fft_cpx> in(Nfft), out(Nfft);

    for (int i = 0; i < N; ++i) {
        in[i].r = mbn[i];
        in[i].i = 0;
    }
    for (int i = N; i < Nfft; ++i) {
        in[i].r = in[i].i = 0;
    }

    kiss_fft(cfg, in.data(), out.data());
    kiss_fft_free(cfg);

    int M = Nfft/2 + 1;
    QVector<double> P1(M), freq(M);
    for (int i = 0; i < M; ++i) {
        double mag = std::hypot(out[i].r, out[i].i) / Nfft;
        if (i != 0 && i != Nfft/2)
            mag *= 2.0;
        P1[i]   = mag;
        freq[i] = double(fs) * i / Nfft;
    }

    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < M; ++i) {
        series->append(freq[i], P1[i]);
    }
    chart->addSeries(series);

    chart->createDefaultAxes();
    chart->axisX()->setTitleText("Frequency (Hz)");
    chart->axisY()->setTitleText("Amplitude");
    ui->widget->setChart(chart);
    ui->widget->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::plotEnvelope(int /*index*/)
{
    QChart *chart = new QChart();
    chart->setTitle("Signals & Envelopes");

    for (int i = 0; i < mbnMatrix.size(); ++i) {
        QLineSeries *sRaw = new QLineSeries();
        QLineSeries *sEnv = new QLineSeries();
        sRaw->setName(QString("MBN Raw %1").arg(i + 1));
        sEnv->setName(QString("Envelope %1").arg(i + 1));

        const DoubleVector &raw = mbnMatrix[i];
        const DoubleVector &env = envelopeMatrix[i];
        for (int j = 0; j < raw.size(); ++j) {
            sRaw->append(j, raw[j]);
            sEnv->append(j, env[j]);
        }

        chart->addSeries(sRaw);
        chart->addSeries(sEnv);
    }

    chart->createDefaultAxes();

    chart->axisX()->setTitleText("Time (sample index)");
    chart->axisY()->setTitleText("Amplitude");

    ui->widget->setChart(chart);
    ui->widget->setRenderHint(QPainter::Antialiasing);
}
