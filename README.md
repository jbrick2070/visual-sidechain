# Visual Sidechain

**Visual Sidechain** is an FFT-reactive GPU shader filter suite and sibling-filter Audio Modulator for OBS Studio 33 on Windows x64 (NVIDIA RTX first). By default it taps **Track 1 (Main Mix)** (or any individual OBS audio source / mixer track), runs a real-time 2048-point Hann-windowed FFT, uploads a 512-bin GPU FFT spectrum + waveform texture (`vsc_fft_tex`) and 32-band logarithmic spectrum texture (`vsc_bands_tex`), and drives 10 original GPU shader filters (**FFT Spectral Aura / Edge Glow**, **FFT Wave Warp & Glitch**, **FFT Chromatic Prism & Bloom**, **FFT Visualizer Overlay**, **FFT Zoom Pulse**, **FFT Kaleidoscope**, **FFT Feedback Trails**, **FFT Strobe & Flash**, **FFT Mirror & Split**, and **Custom FFT Shader**) plus the **Audio Modulator** filter for sidechaining sibling filters including OBS 33's built-in **NVIDIA Blur**, **NVIDIA Background Blur**, and **NVIDIA Background Removal**.

> **Disclaimer:** Visual Sidechain is an independent third-party plugin for OBS Studio. It is not made, endorsed, or supported by the OBS Project or NVIDIA Corporation.

## Development & AI Disclosure

This project is developed with the assistance of AI coding tools under human direction, review, and hardware verification on OBS Studio 33. All shaders are original `GPL-2.0-or-later` implementations with no Shadertoy copies.

## Documentation

- [REQUIREMENTS.md](REQUIREMENTS.md) — Functional, FFT/DSP, GPU shader, NVIDIA RTX modulator, threading, licensing, and testing requirements (`R-*`) and resolved decisions (`D-1..D-4`).
- [PLAN.md](PLAN.md) — Phased implementation plan, OBS 33 & upstream audit findings, 3-layer FFT architecture, test matrix, and risk register.
- [LICENSE](LICENSE) — GNU General Public License v2.0.
