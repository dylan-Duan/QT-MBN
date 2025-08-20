#include "signalprocessor.h"
#include <algorithm>
#include <cmath>
#include <QDebug>


// —————————————— 已有两函数 ——————————————

MBNMatrix processAllMBN(const QList<TableData> &allData)
{
    // 预分配结果容器
    MBNMatrix MBN_all;
    MBN_all.reserve(allData.size());

    const int rows = 100000;  // 每列行数
    const int cols = 10;      // 一共 10 列数据（传感器通道）

    for (const TableData &table : allData)
    {
        // 提取原始 MBN 数据并预分配
        DoubleVector MBN_raw;
        MBN_raw.reserve(rows * cols);

        // 提取第 2 列（索引 1），并忽略少于 4 列的无效行
        for (const RowData &row : table)
        {
            if (row.size() < 4)
                continue;
            MBN_raw << row[1].toDouble();
        }

        // 如果数量不对则警告并跳过
        if (MBN_raw.size() != rows * cols) {
            qWarning() << "MBN size mismatch, skipping table";
            continue;
        }

        // 按行（100 000 行 × 10 列）求平均
        DoubleVector MBN(rows);
        for (int i = 0; i < rows; ++i) {
            double sum = 0.0;
            for (int j = 0; j < cols; ++j) {
                sum += MBN_raw[j * rows + i];
            }
            MBN[i] = sum / cols;
        }

        // 收集结果
        MBN_all << MBN;
    }

    return MBN_all;
}

QVector<double> butterworthFilter(const QVector<double> &x, double cutoffHz, double fs)
{
    int n = x.size();
    QVector<double> y(n);

    double Wn  = std::tan(M_PI * cutoffHz / fs);
    double Wn2 = Wn * Wn;
    double norm = 1.0 + std::sqrt(2.0) * Wn + Wn2;

    double b0 = Wn2 / norm;
    double b1 = 2.0 * b0;
    double b2 = b0;
    double a1 = 2.0 * (Wn2 - 1.0) / norm;
    double a2 = (1.0 - std::sqrt(2.0) * Wn + Wn2) / norm;

    for (int i = 0; i < n; ++i) {
        double yv = b0 * x[i];
        if (i > 0) yv += b1 * x[i - 1] - a1 * y[i - 1];
        if (i > 1) yv += b2 * x[i - 2] - a2 * y[i - 2];
        y[i] = yv;
    }
    return y;
}


MBNMatrix extractEnvelopes(const MBNMatrix &mbnMatrix)
{
    const double Fs = 10000.0;      // 包络采样频率（1 kHz）
    const double cutoffHz = 20.0;   // 低通截止频率 2 Hz

    MBNMatrix filterMBN;
    filterMBN.reserve(mbnMatrix.size());

    for (const DoubleVector &x1 : mbnMatrix) {
        const int N = x1.size();
        QVector<double> x2(N), env(N);

        // Step 1：平方检波（带 2 倍系数）
        for (int j = 0; j < N; ++j)
            x2[j] = 2.0 * x1[j] * x1[j];

        // Step 2：2 阶 Butterworth 低通（双线性变换）
        QVector<double> y = butterworthFilter(x2, cutoffHz, Fs);

        // Step 3：开方还原（再乘 2）
        for (int j = 0; j < N; ++j)
            env[j] = 2.0 * std::sqrt(std::max(0.0, y[j]));

        filterMBN << env;  // 追加到输出矩阵
    }

    return filterMBN;
}


// —————————————— 新增峰值与振铃函数 ——————————————

static double calculateProminence(const QList<double> &sig, int idx) {
    double peak = sig[idx];
    int N = sig.size();
    double minLeft = peak, minRight = peak;
    for (int L = idx - 1; L >= 0 && sig[L] < peak; --L)
        minLeft = std::min(minLeft, sig[L]);
    for (int R = idx + 1; R < N && sig[R] < peak; ++R)
        minRight = std::min(minRight, sig[R]);
    double baseline = std::max(minLeft, minRight);
    return peak - baseline;
}

QVector<PeakInfo> findPeaksWithWidth(const QVector<double> &signal, double fs, double minProminenceRatio)
{
    QVector<PeakInfo> peaks;
    int N = signal.size();
    if (N < 3) return peaks;

    double globalMax = *std::max_element(signal.constBegin(), signal.constEnd());
    double promThresh = globalMax * minProminenceRatio;

    for (int i = 1; i < N - 1; ++i) {
        if (signal[i] > signal[i - 1] && signal[i] > signal[i + 1]) {
            double prom = calculateProminence(signal, i);
            if (prom >= promThresh) {
                double half = signal[i] / 2.0;
                int L = i, R = i;
                while (L > 0   && signal[L] > half)  --L;
                while (R < N-1 && signal[R] > half) ++R;

                double widthSec = double(R - L) / fs;
                peaks.append({ signal[i], widthSec, signal[i] / widthSec });

                int j = i + 1;
                while (j < N && signal[j] == signal[i]) ++j;
                i = j - 1;
            }
        }
    }

    // 检查最后一个点是否为峰
    if (signal.size() >= 2) {
        int i = N - 1;
        if (signal[i] > signal[i - 1]) {
            double prom = calculateProminence(signal, i);
            if (prom >= promThresh) {
                double half = signal[i] / 2.0;
                int L = i;
                while (L > 0 && signal[L] > half) --L;

                double widthSec = double(i - L) / fs;
                peaks.append({ signal[i], widthSec, signal[i] / widthSec });
            }
        }
    }

    return peaks;
}


int countRingingByPeaks(const QVector<double> &x, double thresholdRatio = 0.01)
{
    int N = x.size();
    if (N < 2) return 0;

    // 计算幅值门限 epsVal = thresholdRatio * max(|x|)
    double maxAbs = 0.0;
    for (double v : x) {
        maxAbs = std::max(maxAbs, std::abs(v));
    }
    double epsVal = thresholdRatio * maxAbs;

    int posCount = 0, negCount = 0;

    // 起点 i=0
    if (x[0] > x[1] && x[0] > epsVal) ++posCount;
    if (-x[0] > -x[1] && -x[0] > epsVal) ++negCount;

    // 主循环 i=1..N-2：正峰与负峰；对平台做消重
    for (int i = 1; i < N - 1; ++i) {
        if (x[i] > x[i - 1] && x[i] > x[i + 1] && x[i] > epsVal) {
            ++posCount;
            int j = i + 1; while (j < N && x[j] == x[i]) ++j;
            i = j - 1;
            continue;
        }
        double v = -x[i];
        if (v > -x[i - 1] && v > -x[i + 1] && v > epsVal) {
            ++negCount;
            int j = i + 1; while (j < N && -x[j] == v) ++j;
            i = j - 1;
        }
    }

    // 终点 i=N-1
    if (x[N - 1] > x[N - 2] && x[N - 1] > epsVal) ++posCount;
    if (-x[N - 1] > -x[N - 2] && -x[N - 1] > epsVal) ++negCount;

    // 1 次振铃 ≈ 一个正峰 + 一个负峰，向上取整
    int totalPeaks = posCount + negCount;
    return static_cast<int>(std::ceil(totalPeaks / 2.0));
}
void analyzeAllPeaks(const MBNMatrix &envelopes,
                     double fs, double minProminenceRatio, double ringingThresholdRatio)
{
    for (int idx = 0; idx < envelopes.size(); ++idx) {
        const DoubleVector &env = envelopes[idx];

        // 1) 峰值检测（幅值、FWHM(秒)、幅宽比）
        QVector<PeakInfo> peaks = findPeaksWithWidth(env, fs, minProminenceRatio);

        // 2) 振铃次数
        int ringing = countRingingByPeaks(env, ringingThresholdRatio);

        // 3) 打印
        qDebug().noquote() << QString("=== Signal %1 Peak Features ===").arg(idx + 1);
        qDebug().noquote() << QString("Ringing Count = %1").arg(ringing);

        for (int j = 0; j < peaks.size(); ++j) {
            const PeakInfo &p = peaks[j];
            qDebug().noquote()
                << QString("峰 %1: 幅值=%2, FWHM=%3 s, 峰宽比=%4")
                       .arg(j + 1)
                       .arg(p.amplitude, 0, 'f', 6)
                       .arg(p.fwhm,      0, 'f', 6)
                       .arg(p.ratio,     0, 'f', 6);
        }
    }
}
