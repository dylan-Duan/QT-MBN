#pragma once

#include <QMainWindow>
#include "signalprocessor.h"  // 你需要的类型定义
#include "dbloader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();  // 仅声明

private slots:
    void on_btnBrowse_clicked();
    void on_btnLoad_clicked();
    void on_btnPlotTime_clicked();
    void on_btnPlotFreq_clicked();
    void on_btnPlotEnv_clicked();

private:
    Ui::MainWindow *ui;
    MBNMatrix mbnMatrix;
    MBNMatrix envelopeMatrix;
    int currentIndex = 0;
    void log(const QString &s);
    void plotTimeDomain(int index);
    void plotFrequencySpectrum(int index);
    void plotEnvelope(int index);
    void analyzeSignalFeatures(int index);
};



QVector<PeakInfo> findPeaksWithWidth(const QVector<double> &signal, double fs, double minProminenceRatio);
int countRingingByPeaks(const QVector<double> &x, double thresholdRatio = 0.01);
