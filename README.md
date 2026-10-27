# Visual Sidechain

**Visual Sidechain** is an FFT-reactive GPU shader filter suite and sibling-filter Audio Modulator for OBS Studio 33 on Windows x64 (NVIDIA RTX first). By default it taps **Track 1 (the main mix)** (or any individual OBS audio source / mixer track), runs a real-time dual-resolution Hann-windowed FFT (**4096-point** for high-resolution sub-bass and kick detection $\le 250\text{ Hz}$ paired with **2048-point** for low-latency mids and highs), uploads a `512x2` GPU FFT + waveform texture (`audio_fft`) and a `32x1` logarithmic spectrum texture (`audio_spectrum`), and drives 9 original `GPL-2.0-or-later` FFT-reactive shaders (**FFT Spectral Aura / Edge Glow**, **FFT Wave Warp & Glitch**, **FFT Chromatic Prism & Bloom**, **FFT Visualizer Overlay**, **FFT Zoom Pulse**, **FFT Kaleidoscope**, **FFT Feedback Trails**, **FFT Strobe & Flash**, and **FFT Mirror & Split**) plus the **Audio Modulator** filter for sidechaining sibling filters—including OBS 33's built-in **NVIDIA Blur**, **NVIDIA Background Blur**, and **NVIDIA Background Removal**.

> **Disclaimer:** Visual Sidechain is an independent third-party plugin for OBS Studio. It is not made, endorsed, or supported by the OBS Project or NVIDIA Corporation.

## Development & AI Disclosure

This project is written with AI coding tools and reviewed and tested by the author on OBS Studio 33. All bundled shaders are original `GPL-2.0-or-later` implementations with no Shadertoy copies.

## Documentation

- [REQUIREMENTS.md](REQUIREMENTS.md) — Functional, dual-FFT/DSP, GPU shader, NVIDIA RTX modulator, baseline save-protection, threading, licensing, and testing requirements (`R-*`) and decisions (`D-1..D-4`).
- [PLAN.md](PLAN.md) — Phased implementation plan, OBS 33 & upstream audit findings, 3-layer architecture, per-step named tests and mutation checks, risk register, and verify-at-build checklist.
- [LICENSE](LICENSE) — GNU General Public License v2.0.
