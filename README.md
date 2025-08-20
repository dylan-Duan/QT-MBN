# MBN Feature Extraction Tool

This repository provides a complete signal processing pipeline for Magnetic Barkhausen Noise (MBN) analysis. It includes time-domain feature computation, frequency-domain transformation, envelope extraction using a Butterworth filter, peak identification, and GUI-based visualization.

---

## ✅ Features

- **Time-Domain Features**: Mean value, RMS, number of Ringing events
- **Frequency-Domain Analysis**: Zero-padded FFT, single-sided spectrum, energy scaling
- **Envelope Extraction**: Square–lowpass–sqrt chain with a 2nd-order Butterworth filter
- **Peak Identification**: Amplitude, FWHM (Full Width at Half Maximum), relative ratio
- **Repeatability & Determinism**: All steps are deterministic and reproducible without randomness
- **Graphical User Interface**: Load `.db` files, visualize results, export data

---

## 📁 Project Structure

