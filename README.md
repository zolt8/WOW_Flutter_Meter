# Wow and Flutter Analyzer DLL (C Implementation)

This project is a **C-based DLL implementation** of a wow and flutter analyzer, designed to measure short-term speed variations in audio playback from a test tone (typically 3150 Hz or 3000 Hz).  

It is based on code from the GitHub repository by **Alexander Sibiryakov**, which itself originates from the **original `wfgui.exe` program written by Alex Freed**.

---

## Project History

- **Original Tool:** `wfgui.exe` by Alex Freed  
  - A Windows GUI program for measuring wow and flutter from audio signals.
- **GitHub Repository:** https://github.com/sibiryakov/wow-and-flutter-analyzer  
  - Alexander Sibiryakov extracted and refactored the original code.
- **This Project:**  
  - Extracted selected C code from Sibiryakov's repository  
  - Refactored into a DLL with a simple public API  
  - Designed for easy integration and automated testing

---

## Features

- Measures **wow and flutter** from a recorded test tone
- Supports multiple **weighting filters**:
  - Unweighted  
  - DIN  
  - Wow (low frequency)  
  - Flutter (high frequency)
- Computes:
  - **RMS wow/flutter** (percentage)  
  - **Quasi-peak** value  
  - **Average measured frequency** (Hz)
- Lightweight, portable C implementation
- Works on **mono PCM 16-bit WAV samples**
- Suitable for:
  - Integration in measurement software
  - Automated test
  - Offline analysis

---

## Validation & Testing

This DLL has been **extensively tested** using multiple generated reference signals:

- Pure sine waves  
- Artificial wow/flutter modulated signals  
- Various modulation depths and frequencies  

All results were **compared against the original `wfgui.exe`** program, and the DLL produces **identical results**.
This confirms that the DLL faithfully reproduces the behaviour of the original analyzer.

---

## Building

This is a plain C project and can be compiled with any standard C compiler.

### GCC (Command Line)

gcc -O3 -Wall -c -o flutter_meter.o "..\\flutter_meter.c" 
gcc -O3 -Wall -c -o filters.o "..\\filters.c" 
gcc -shared -o libWFmeter.dll filters.o flutter_meter.o 
