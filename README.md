# Visual Sidechain

**Visual Sidechain** is an audio-reactive video filter suite and parameter modulator for OBS Studio 33 on Windows x64 (NVIDIA RTX first). It taps any OBS audio source (or the main program mix), extracts real-time musical envelopes and beat signals (`level`, `peak`, `bass`, `mid`, `high`, `kick`, `snare`, `beat_phase`, and a 32-band logarithmic spectrum), and drives either built-in GPU shader filters (**Audio-Reactive Glow / Bloom**, **Audio-Reactive Chromatic Aberration**, **Audio-Reactive Zoom / Pulse**, **Audio-Reactive Color / Flash**, **Audio-Reactive Blur / Vignette**, and a **Spectrum / Waveform Overlay**) or the **Audio Modulator** filter, which sidechains any numeric setting on any sibling filter—including OBS 33's built-in NVIDIA RTX Video Effects filters (`Background Removal`, `Blur`, `Background Blur`) and third-party filters—without modifying their code.

> **Disclaimer:** Visual Sidechain is an independent third-party plugin for OBS Studio. It is not made, endorsed, or supported by the OBS Project or NVIDIA Corporation.

## Development & AI Disclosure

This project is developed with the assistance of AI coding tools under human direction, review, and hardware verification on OBS Studio 33.

## Documentation

- [REQUIREMENTS.md](REQUIREMENTS.md) — Functional, architectural, threading, licensing, and testing requirements (`R-*`) and open decisions (`D-1..D-4`).
- [PLAN.md](PLAN.md) — Phased implementation plan, upstream & OBS 33 source findings, test matrix, risk register, and decision table.
- [LICENSE](LICENSE) — GNU General Public License v2.0.
