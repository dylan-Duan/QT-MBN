#pragma once
#include <QVector>
#include <QList>
#include <QVariant>
#include <cmath>

using DoubleVector = QVector<double>;
using MBNMatrix = QVector<DoubleVector>;
using TableData = QList<QList<QVariant>>;
using RowData = QList<QVariant>;

struct PeakInfo {
    double amplitude;
    double fwhm;
    double ratio;
};

MBNMatrix processAllMBN(const QList<TableData> &allData);
MBNMatrix extractEnvelopes(const MBNMatrix &mbnMatrix);
// 打印每路包络的峰值特征（幅值、FWHM、幅宽比）及振铃次数
void analyzeAllPeaks(const MBNMatrix &envelopes,
                     double fs = 100000.0,
                     double minProminenceRatio = 0.2,
                     double ringingThresholdRatio = 0.01);
QVector<double> butterworthFilter(const QVector<double> &x, double cutoffHz, double fs);


// 下面两个要用 QVector<double> 和你 MainWindow 里传的一致
QVector<PeakInfo> findPeaksWithWidth(const QVector<double> &signal,
                                    double fs,
                                    double minProminenceRatio);
int countRingingByPeaks(const QVector<double> &x,
                          double thresholdRatio);

