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
MBN-Feature-Extraction/
│
├── main.py # GUI entry point
├── requirements.txt # Python dependencies
├── mbn/ # Core signal processing modules
│ ├── signal_io.py
│ ├── time_features.py
│ ├── freq_analysis.py
│ ├── envelope.py
│ ├── peak_detection.py
│ └── utils.py
├── ui/ # PyQt UI layout files
├── examples/ # Example .db files
└── docs/ # (Optional) Usage guides

---
🧪 Sample Output

For each input .db file, the system extracts the following MBN features:

Mean_Value, RMS_Value, and Number of Ringing events

Top envelope peaks with Amplitude, FWHM, and Relative Ratio

Optional FFT analysis and envelope visualization

🖼 Screenshots

GUI layout, signal plots, and output results
(Insert figures here in your GitHub project)

📘 Future Work

This version supports only one envelope extraction method (square–lowpass–sqrt). Future updates may include comparisons with Hilbert and other demodulation techniques. Support for automatic .db parsing and batch processing will also be added to improve usability.

📄 License

This project is licensed under the MIT License. You are free to use, modify, and distribute this code for academic or commercial purposes.

📫 Contact

For bug reports, suggestions, or collaboration, please open an issue on GitHub or contact the project maintainer.

This repository supports the results published in [Design and Implementation of a Magnetic Barkhausen Noise Detection System Based on QT], 2025.
