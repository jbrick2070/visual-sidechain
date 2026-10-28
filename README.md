# Visual Sidechain

The coordinated development version is **0.6.0**, shared with Midi Fadester and Fadester for OBS. Install and update each plugin separately; this source version does not create a release tag or replace the operator's hardware/GPU acceptance.

**Visual Sidechain** is an FFT-reactive GPU shader filter suite and sibling-filter Audio Modulator for OBS Studio 33 on Windows x64 (NVIDIA RTX first). By default it taps **Track 1 (the main mix)** (or any individual OBS audio source / mixer track), runs a real-time dual-resolution Hann-windowed FFT (**4096-point** for high-resolution sub-bass and kick detection $\le 250\text{ Hz}$ paired with **2048-point** for low-latency mids and highs), uploads a `512x2` GPU FFT + waveform texture (`audio_fft`) and a `32x1` logarithmic spectrum texture (`audio_spectrum`), and drives 11 original `GPL-2.0-or-later` FFT-reactive shaders (**FFT Spectral Edge / Border Aura**, **FFT Wave Warp & Glitch**, **FFT Chromatic Prism & Bloom**, **FFT Visualizer Overlays**, **FFT Zoom Pulse & Barrel Lens**, **FFT Kaleidoscope**, **FFT Feedback Trails**, **FFT Strobe & Flash**, **FFT Mirror & Split**, **FFT Hex Pulse Lattice**, and **FFT Polygon Tunnel**) plus the **Audio Modulator** filter for sidechaining sibling filters—including OBS 33's built-in **NVIDIA Blur**, **NVIDIA Background Blur**, and **NVIDIA Background Removal**. All audio and beat signals are also available over the `obs-websocket` 5 vendor API (`vendor_name = "visual-sidechain"`), and all 18 Fadester slots (`crossfader`, `fader_1`..`fader_8`, `master`, `fx_1`..`fx_8`) can act as modulation sources in the Audio Modulator and as float uniforms (`fader_crossfader`, `fader_1`..`fader_8`, `fader_master`, `fader_fx_1`..`fader_fx_8`) in shaders.

> **Disclaimer:** Visual Sidechain is an independent third-party plugin for OBS Studio. It is not made, endorsed, or supported by the OBS Project or NVIDIA Corporation.

## Installation (OBS Studio 33.0.0+ on Windows x64)

Download the latest release from [GitHub Releases](https://github.com/jbrick2070/visual-sidechain/releases):

- **Standard Windows Install (`*-Windows-Installer.exe` or `*-Windows.zip`):** Run the installer or extract `visual-sidechain/` into `C:\ProgramData\obs-studio\plugins\` (OBS 33 unified plugin package layout: `visual-sidechain\visual-sidechain.dll` + `visual-sidechain\data\...`).
- **Portable OBS 33 Install (`*-Portable.zip`):** Extract `plugins\visual-sidechain\` directly into the root of your portable OBS Studio 33 folder.
- **Unsigned Binary Note (Windows SmartScreen / Smart App Control):** Release binaries are not Authenticode code-signed. If Windows SmartScreen prompts on the `.exe` installer, click **More info -> Run anyway** (or if unloading a downloaded `.zip`, right-click the `.zip` -> **Properties -> Unblock** before extracting). Systems running Windows 11 **Smart App Control** in strict enforcement mode block unsigned third-party DLLs by OS policy.

## Quick Start & Photosensitivity Safety Hotkeys

1. Right-click any video source or scene in OBS -> **Filters** -> **Effect Filters (`+`)**:
   - Add **`Visual Sidechain: Shader Filter`** to apply any of the 11 built-in FFT-reactive GPU shaders or load your own `.shader` / `.effect` file. **`Effect Mix (Dry / Wet)`** and **`Audio Reactivity`** sliders sit under the preset and work without any MIDI hardware; the audio analysis group sits at the bottom of the panel with its advanced tuning hidden behind **`Show Advanced Analysis Settings`**. Under **`Fader Control (Midi & Stream Deck Fadester)`**, you can bind any Fadester slot (`Crossfader`, `Fader 1..8`, `Master`, `FX 1..8`) to **`Effect Mix Fader (Dry / Wet)`**, **`Audio Reactivity Fader`**, or up to two direct **`Parameter Fader`** targets (auto-seeded to the parameter's range and clamped to safety ceilings).
   - Add **`Visual Sidechain: Audio Modulator`** alongside any sibling filter (such as **Color Correction**, **NVIDIA Background Removal**, or **NVIDIA Blur**) to drive numeric settings from `Kick Onset`, `Bass`, `Beat Phase`, any `Spectrum Band 1..32`, a `Custom Hz Window`, or a `Midi Fadester` fader slot—and optionally assign a **`Depth Fader`** per slot to scale `Modulation Amount` live from `0–100%` from a MIDI fader or Stream Deck + dial. Target `Minimum` / `Maximum` bounds are auto-seeded when you pick a target setting, and original baselines are automatically restored on disable, removal, or scene save.
2. **Photosensitivity & Panic Hotkeys (`Settings -> Hotkeys`):**
   - **`Visual Sidechain: Panic`** immediately puts every `Visual Sidechain: Shader Filter` (including custom user shaders) into pass-through and zeroes all strobe, flash, and modulation amounts across OBS.
   - **`Visual Sidechain: Restore`** exits pass-through and restores pre-panic amounts.
   - `FFT Strobe & Flash` defaults to a **3.0 Hz** maximum flash rate cap per photosensitivity guidelines.

## Geometric presets and fader assignments

**FFT Hex Pulse Lattice** draws honeycomb outlines over your source. Bass expands the cells, kick drives a spatial ripple, and highs shift the edge accents. Try a Parameter Fader on the cell density, with the second fader controlling overlay opacity.

**FFT Polygon Tunnel** draws nested polygons with continuous depth motion. Choose three through eight sides, then adjust tunnel depth, travel speed, and rotation. Try binding tunnel depth to a Parameter Fader. Both presets preserve source transparency and remain visible during silence; set audio response to zero for a purely geometric look.

Assignment summaries show the selected fader, destination, and range. Reset buttons affect only their named binding: a parameter reset clears its target and custom range, while a Modulator depth reset keeps the target, driving signal, and amount. Within a Shader Filter, the first parameter assignment owns its target even when it uses the slider value. Within an Audio Modulator, the first enabled slot owns each filter/setting pair. Conflicting choices are disabled, and duplicate assignments loaded from settings are cleared by the existing ownership rules.

Custom shader parameters can use `effect_mix` and `audio_reactivity` independently of the host's percent sliders. Names that collide with internal controls, such as `preset_shader`, `custom_low_hz`, or `vs_effect_mix_pct`, produce a compile message identifying the parameter to rename. A failed shader passes the source through until a valid shader is loaded. In full-effect mode, `vs_effect_mix` and `vs_reactivity_scale` are host-supplied uniforms, so they do not appear as editable shader parameters.

## `obs-websocket` 5 Vendor API

### Midi Fadester and Stream Deck integration

Midi Fadester 0.4.0 and Fadester for OBS expose an 18-slot desk. All 18 slots are available as Shader Filter mix/reactivity/parameter controls and Audio Modulator signal/depth sources. Existing saved fader indices `0..9` and modulation signal IDs `200..209` stay unchanged; the FX bank appends indices `10..17` and signal IDs `210..217`. The in-process bridge uses the same slot IDs as Stream Deck. Older Midi Fadester providers continue to supply their ten legacy slots; unsupported FX slots read zero.

For setup from the Stream Deck Property Inspector, assign a Midi slot to **Filter setting (FX)**, select the Visual Sidechain filter, then choose a numeric setting from its catalog. Audio Modulator depth trims are `slot_1_amount_pct` through `slot_8_amount_pct`: choose **At 0 % = 0** and **At 100 % = 100** to remove audio-driven movement at the zero rail. Amount zero holds the modulator target at its configured Minimum; choose that Minimum as the desired neutral setting. The property itself supports `-100..100`; using that full range makes the zero rail maximum ducking. For ducking, keep a negative Modulation Amount and bind a Depth Fader instead of sweeping the amount across zero, so zero depth retains the Maximum endpoint of the ducking branch. Shader Filter's host percent controls are `vs_effect_mix_pct` and `vs_audio_reactivity_pct`; custom numeric shader parameters also appear in the catalog.

Midi's **Clean / All FX Off** resets enabled `filter_param` bindings in either bank to their configured zero endpoint. It does not reset arbitrary audio/layer assignments or invoke Visual Sidechain's separate global **Panic / Restore** state. A numeric trim reset changes that trim; enabling **Bypass at 0 %** additionally bypasses the entire target filter, with shared ownership managed by Midi Fadester. Avoid assigning two control paths to the same setting: use either the filter's internal fader assignment or a Midi `filter_param` binding for that setting.

- **Vendor Name:** `"visual-sidechain"`
- **Request:** `CallVendorRequest` with `vendorName: "visual-sidechain"`, `requestType: "get_signals"` returns `{ success, version, api_version: 1, level, sub_bass, bass, mid, high, kick, snare, beat, beat_phase, spectral_centroid, spectral_flux, estimated_bpm, bpm, spectrum: [{ band: 0..31, value }] }`.
- **Event:** `VendorEvent` (`vendorName: "visual-sidechain"`, `eventType: "signals_changed"`) streams the same payload coalesced to at most 30 events/sec.

## Development & AI Disclosure

This project is written with AI coding tools and reviewed and tested by the author on OBS Studio 33. All bundled shaders are original `GPL-2.0-or-later` implementations with no Shadertoy copies.

## Documentation & Beta Feedback

- **[BETA-TESTING.md](BETA-TESTING.md)** — **10-minute Beta Tester Guide & Hardware Checklist** for OBS Studio 33 ([Submit a Beta Report](https://github.com/jbrick2070/visual-sidechain/issues/new?template=beta-feedback.yml)).
- [REQUIREMENTS.md](REQUIREMENTS.md) — Functional, dual-FFT/DSP, GPU shader, NVIDIA RTX modulator, baseline save-protection, threading, licensing, and testing requirements (`R-*`) and decisions (`D-1..D-4`).
- [PLAN.md](PLAN.md) — Phased implementation plan, OBS 33 & upstream audit findings, 3-layer architecture, per-step named tests and mutation checks, risk register, and verify-at-build checklist.
- [TESTING.md](TESTING.md) — Headless CTest, mutation-check, and live OBS Studio 33.0.0 hardware verification results.
- [LICENSE](LICENSE) — GNU General Public License v2.0.
