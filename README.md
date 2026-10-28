# Visual Sidechain

**Visual Sidechain** is an FFT-reactive GPU shader filter suite and sibling-filter Audio Modulator for OBS Studio 33 on Windows x64 (NVIDIA RTX first). By default it taps **Track 1 (the main mix)** (or any individual OBS audio source / mixer track), runs a real-time dual-resolution Hann-windowed FFT (**4096-point** for high-resolution sub-bass and kick detection $\le 250\text{ Hz}$ paired with **2048-point** for low-latency mids and highs), uploads a `512x2` GPU FFT + waveform texture (`audio_fft`) and a `32x1` logarithmic spectrum texture (`audio_spectrum`), and drives 9 original `GPL-2.0-or-later` FFT-reactive shaders (**FFT Spectral Edge / Border Aura**, **FFT Wave Warp & Glitch**, **FFT Chromatic Prism & Bloom**, **FFT Visualizer Overlays**, **FFT Zoom Pulse & Barrel Lens**, **FFT Kaleidoscope**, **FFT Feedback Trails**, **FFT Strobe & Flash**, and **FFT Mirror & Split**) plus the **Audio Modulator** filter for sidechaining sibling filters—including OBS 33's built-in **NVIDIA Blur**, **NVIDIA Background Blur**, and **NVIDIA Background Removal**. All audio and beat signals are also available over the `obs-websocket` 5 vendor API (`vendor_name = "visual-sidechain"`), and the 10 Fadester fader slots (`crossfader`, `fader_1`..`fader_8`, `master`) can act as modulation sources in the Audio Modulator and as float uniforms (`fader_crossfader`, `fader_1`..`fader_8`, `fader_master`) in shaders.

> **Disclaimer:** Visual Sidechain is an independent third-party plugin for OBS Studio. It is not made, endorsed, or supported by the OBS Project or NVIDIA Corporation.

## Installation (OBS Studio 33.0.0+ on Windows x64)

Download the latest release from [GitHub Releases](https://github.com/jbrick2070/visual-sidechain/releases):

- **Standard Windows Install (`*-Windows-Installer.exe` or `*-Windows.zip`):** Run the installer or extract `visual-sidechain/` into `C:\ProgramData\obs-studio\plugins\` (OBS 33 unified plugin package layout: `visual-sidechain\visual-sidechain.dll` + `visual-sidechain\data\...`).
- **Portable OBS 33 Install (`*-Portable.zip`):** Extract `plugins\visual-sidechain\` directly into the root of your portable OBS Studio 33 folder.
- **Unsigned Binary Note (Windows SmartScreen / Smart App Control):** Release binaries are not Authenticode code-signed. If Windows SmartScreen prompts on the `.exe` installer, click **More info -> Run anyway** (or if unloading a downloaded `.zip`, right-click the `.zip` -> **Properties -> Unblock** before extracting). Systems running Windows 11 **Smart App Control** in strict enforcement mode block unsigned third-party DLLs by OS policy.

## Quick Start & Photosensitivity Safety Hotkeys

1. Right-click any video source or scene in OBS -> **Filters** -> **Effect Filters (`+`)**:
   - Add **`Visual Sidechain: Shader Filter`** to apply any of the 9 built-in FFT-reactive GPU shaders or load your own `.shader` / `.effect` file.
   - Add **`Visual Sidechain: Audio Modulator`** alongside any sibling filter (such as **Color Correction**, **NVIDIA Background Removal**, or **NVIDIA Blur**) to drive numeric settings from `Kick Onset`, `Bass`, `Beat Phase`, any `Spectrum Band 1..32`, a `Custom Hz Window`, or a `Midi Fadester` fader slot. Target `Minimum` / `Maximum` bounds are auto-seeded when you pick a target setting, and original baselines are automatically restored on disable, removal, or scene save.
2. **Photosensitivity & Panic Hotkeys (`Settings -> Hotkeys`):**
   - **`Visual Sidechain: Panic`** immediately puts every `Visual Sidechain: Shader Filter` (including custom user shaders) into pass-through and zeroes all strobe, flash, and modulation amounts across OBS.
   - **`Visual Sidechain: Restore`** exits pass-through and restores pre-panic amounts.
   - `FFT Strobe & Flash` defaults to a **3.0 Hz** maximum flash rate cap per photosensitivity guidelines.

## `obs-websocket` 5 Vendor API

- **Vendor Name:** `"visual-sidechain"`
- **Request:** `CallVendorRequest` with `vendorName: "visual-sidechain"`, `requestType: "get_signals"` returns `{ success, version, api_version: 1, level, sub_bass, bass, mid, high, kick, snare, beat, beat_phase, spectral_centroid, spectral_flux, estimated_bpm, bpm, spectrum: [{ band: 0..31, value }] }`.
- **Event:** `VendorEvent` (`vendorName: "visual-sidechain"`, `eventType: "signals_changed"`) streams the same payload coalesced to at most 30 events/sec.

## Development & AI Disclosure

This project is written with AI coding tools and reviewed and tested by the author on OBS Studio 33. All bundled shaders are original `GPL-2.0-or-later` implementations with no Shadertoy copies.

## Documentation

- [REQUIREMENTS.md](REQUIREMENTS.md) — Functional, dual-FFT/DSP, GPU shader, NVIDIA RTX modulator, baseline save-protection, threading, licensing, and testing requirements (`R-*`) and decisions (`D-1..D-4`).
- [PLAN.md](PLAN.md) — Phased implementation plan, OBS 33 & upstream audit findings, 3-layer architecture, per-step named tests and mutation checks, risk register, and verify-at-build checklist.
- [TESTING.md](TESTING.md) — Headless CTest, mutation-check, and live OBS Studio 33.0.0 hardware verification results.
- [LICENSE](LICENSE) — GNU General Public License v2.0.
