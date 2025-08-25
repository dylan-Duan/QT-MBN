#include "signalprocessor.h"
#include <algorithm>
#include <cmath>
#include <QDebug>


// —————————————— Existing two functions ——————————————

MBNMatrix processAllMBN(const QList<TableData> &allData)
{
    // Pre-allocate result container
    MBNMatrix MBN_all;
    MBN_all.reserve(allData.size());

    const int rows = 100000;  // number of rows per column
    const int cols = 5;       // total of 10 columns of data (sensor channels)

    for (const TableData &table : allData)
    {
        // Extract raw MBN data and pre-allocate
        DoubleVector MBN_raw;
        MBN_raw.reserve(rows * cols);

        // Extract the 2nd column (index 1), skip invalid rows with less than 4 columns
        for (const RowData &row : table)
        {
            if (row.size() < 4)
                continue;
            MBN_raw << row[1].toDouble();
        }

        // If size mismatch, warn and skip
        if (MBN_raw.size() != rows * cols) {
            qWarning() << "MBN size mismatch, skipping table";
            continue;
        }

        // Row-wise average (100,000 rows × 10 columns)
        DoubleVector MBN(rows);
        for (int i = 0; i < rows; ++i) {
            double sum = 0.0;
            for (int j = 0; j < cols; ++j) {
                sum += MBN_raw[j * rows + i];
            }
            MBN[i] = sum / cols;
        }

        // Collect results
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
    const double Fs = 10000.0;      // envelope sampling frequency (1 kHz)
    const double cutoffHz = 20.0;   // low-pass cutoff frequency 2 Hz

    MBNMatrix filterMBN;
    filterMBN.reserve(mbnMatrix.size());

    for (const DoubleVector &x1 : mbnMatrix) {
        const int N = x1.size();
        QVector<double> x2(N), env(N);

        // Step 1: square detection (with factor 2)
        for (int j = 0; j < N; ++j)
            x2[j] = 2.0 * x1[j] * x1[j];

        // Step 2: 2nd-order Butterworth low-pass (bilinear transform)
        QVector<double> y = butterworthFilter(x2, cutoffHz, Fs);

        // Step 3: square root recovery (multiply by 2 again)
        for (int j = 0; j < N; ++j)
            env[j] = 2.0 * std::sqrt(std::max(0.0, y[j]));

        filterMBN << env;  // append to output matrix
    }

    return filterMBN;
}


// —————————————— New peak and ringing functions ——————————————

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

    // Check if the last point is a peak
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

    // Compute amplitude threshold epsVal = thresholdRatio * max(|x|)
    double maxAbs = 0.0;
    for (double v : x) {
        maxAbs = std::max(maxAbs, std::abs(v));
    }
    double epsVal = thresholdRatio * maxAbs;

    int posCount = 0, negCount = 0;

    // Start point i=0
    if (x[0] > x[1] && x[0] > epsVal) ++posCount;
    if (-x[0] > -x[1] && -x[0] > epsVal) ++negCount;

    // Main loop i=1..N-2: positive and negative peaks; deduplicate flat regions
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

    // End point i=N-1
    if (x[N - 1] > x[N - 2] && x[N - 1] > epsVal) ++posCount;
    if (-x[N - 1] > -x[N - 2] && -x[N - 1] > epsVal) ++negCount;

    // One ringing ≈ one positive peak + one negative peak, round up
    int totalPeaks = posCount + negCount;
    return static_cast<int>(std::ceil(totalPeaks / 2.0));
}
void analyzeAllPeaks(const MBNMatrix &envelopes,
                     double fs, double minProminenceRatio, double ringingThresholdRatio)
{
    for (int idx = 0; idx < envelopes.size(); ++idx) {
        const DoubleVector &env = envelopes[idx];

        // 1) Peak detection (amplitude, FWHM (s), amplitude-to-width ratio)
        QVector<PeakInfo> peaks = findPeaksWithWidth(env, fs, minProminenceRatio);

        // 2) Ringing count
        int ringing = countRingingByPeaks(env, ringingThresholdRatio);

        // 3) Print
        qDebug().noquote() << QString("=== Signal %1 Peak Features ===").arg(idx + 1);
        qDebug().noquote() << QString("Ringing Count = %1").arg(ringing);

        for (int j = 0; j < peaks.size(); ++j) {
            const PeakInfo &p = peaks[j];
            qDebug().noquote()
                << QString("Peak %1: Amplitude=%2, FWHM=%3 s, Ratio=%4")
                       .arg(j + 1)
                       .arg(p.amplitude, 0, 'f', 6)
                       .arg(p.fwhm,      0, 'f', 6)
                       .arg(p.ratio,     0, 'f', 6);
        }
    }
}
