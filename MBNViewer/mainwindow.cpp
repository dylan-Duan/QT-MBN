#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtGlobal>
#include <kiss_fft.h>      // 记得在.pro里添加 INCLUDEPATH 和 LIBS
#include <QTemporaryDir>  // ← 新增：用于创建临时目录


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
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

    // 2. 振铃次数：先转换为 QList<double>
    QList<double> mbnList = mbn.toList();
    int ringCount = countRingingByPeaks(mbnList, 0.02);

    // 3. 输出 Mean/RMS/振铃次数
    log(QString("=== Signal %1 Features ===").arg(index + 1));
    log(QString("Mean_Value[%1] = %2").arg(index).arg(meanVal, 0, 'f', 15));
    log(QString("RMS_Value[%1]  = %2").arg(index).arg(rmsVal,  0, 'f', 7));
    log(QString("Number of ring = %1").arg(ringCount));

    // 4. 包络峰值特征（峰宽比等）
    if (index < envelopeMatrix.size()) {
        // 转换 envelope 到 QList<double>
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
    // 只支持选择单个 .db 文件
    QString file = QFileDialog::getOpenFileName(this, "选择单个 .db 文件", QString(), "SQLite DB (*.db)");
    if (file.isEmpty()) return;
    ui->editDir->setText(file);
}

void MainWindow::on_btnLoad_clicked()
{
    QString path = ui->editDir->text();
    if (path.isEmpty()) {
        log("文件路径为空");
        return;
    }

    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        log("指定路径不是有效的文件");
        return;
    }


    // 复制到临时目录，再用你现有的 loadAllDbFiles(目录)
    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        log("创建临时目录失败，无法加载文件");
        return;
    }

    const QString dstPath = tmpDir.path() + QLatin1Char('/') + fi.fileName();
    // 若目标已存在先移除（理论上临时目录是空的，此处稳妥处理）
    if (QFile::exists(dstPath)) QFile::remove(dstPath);
    if (!QFile::copy(path, dstPath)) {
        log(QString("复制文件到临时目录失败：%1").arg(path));
        return;
    }

    // 现在临时目录中只有这个 .db 文件；调用原函数只会加载这一份
    auto allData = loadAllDbFiles(tmpDir.path());
    log(QString("实际加载 %1 个 .db 文件").arg(allData.size()));
    if (allData.isEmpty()) {
        log("未加载任何数据：文件无法打开或无数据");
        return;
    }

    mbnMatrix      = processAllMBN(allData);
    envelopeMatrix = extractEnvelopes(mbnMatrix);
    log(QString("处理得到 %1 个有效 MBN 信号").arg(mbnMatrix.size()));

    if (!mbnMatrix.isEmpty()) {
        currentIndex = 0;
        plotTimeDomain(currentIndex);
        analyzeSignalFeatures(currentIndex);
    }
}



void MainWindow::on_btnPlotTime_clicked()
{
    if (mbnMatrix.isEmpty()) {
        log("未检测到数据");
        return;
    }
    plotTimeDomain(currentIndex);
    analyzeSignalFeatures(currentIndex);
}

void MainWindow::on_btnPlotFreq_clicked()
{
    if (mbnMatrix.isEmpty()) {
        log("未检测到数据");
        return;
    }
    plotFrequencySpectrum(currentIndex);
    // 频谱一般不再输出时间域特征
}

void MainWindow::on_btnPlotEnv_clicked()
{
    if (envelopeMatrix.isEmpty()) {
        log("未检测到数据");
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

    // ✅ 添加横纵坐标标题
    chart->axisX()->setTitleText("Time (sample index)");
    chart->axisY()->setTitleText("MBN Amplitude");

    ui->widget->setChart(chart);
}


void MainWindow::plotFrequencySpectrum(int index)
{
    if (index < 0 || index >= mbnMatrix.size())
        return;

    // 新建图表
    QChart *chart = new QChart();
    chart->setTitle("Frequency Spectrum");

    // 参数：采样率 fs = 100kHz
    const int fs = 100000;
    const DoubleVector &mbn = mbnMatrix[index];
    int N = mbn.size();
    if (N == 0) {
        ui->widget->setChart(chart);
        return;
    }

    // 零填充到下一个 2 的幂 Nfft
    int Nfft = 1 << static_cast<int>(std::ceil(std::log2(N)));

    // 分配 FFT 配置和缓冲区
    kiss_fft_cfg cfg = kiss_fft_alloc(Nfft, 0, nullptr, nullptr);
    std::vector<kiss_fft_cpx> in(Nfft), out(Nfft);

    // 填充输入：实部 = 数据，虚部 = 0
    for (int i = 0; i < N; ++i) {
        in[i].r = mbn[i];
        in[i].i = 0;
    }
    for (int i = N; i < Nfft; ++i) {
        in[i].r = in[i].i = 0;
    }

    // 执行 FFT
    kiss_fft(cfg, in.data(), out.data());
    kiss_fft_free(cfg);

    // 计算单边幅度谱和对应频率
    int M = Nfft/2 + 1;
    QVector<double> P1(M), freq(M);
    for (int i = 0; i < M; ++i) {
        double mag = std::hypot(out[i].r, out[i].i) / Nfft;
        if (i != 0 && i != Nfft/2)
            mag *= 2.0;
        P1[i]   = mag;
        freq[i] = double(fs) * i / Nfft;
    }

    // 绘制幅度谱曲线
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < M; ++i) {
        series->append(freq[i], P1[i]);
    }
    chart->addSeries(series);

    // 坐标轴与渲染设置
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

    // ✅ 添加横纵坐标标题
    chart->axisX()->setTitleText("Time (sample index)");
    chart->axisY()->setTitleText("Amplitude");

    ui->widget->setChart(chart);
    ui->widget->setRenderHint(QPainter::Antialiasing);
}

