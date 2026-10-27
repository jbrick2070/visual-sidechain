# Visual Sidechain — Requirements

**Visual Sidechain** is an open-source audio-reactive FFT shader filter suite and sibling-filter parameter modulator for OBS Studio 33 on Windows x64, designed NVIDIA RTX first.

---

## 1. Identity, Branding, and Disclosure (`R-ID-*`)

- **`R-ID-1` (Plugin Name):** The human-readable name is **Visual Sidechain**. The binary module name is `visual-sidechain` (`visual-sidechain.dll`).
- **`R-ID-2` (No "OBS" in Branding):** The word `"OBS"` MUST NOT appear in the plugin's title, branding, repository name, or display names. Display names of filters registered with `obs_register_source` use the `"Visual Sidechain — ..."` prefix.
- **`R-ID-3` (Third-Party Disclaimer):** `README.md`, release notes, and documentation MUST include the explicit statement:
  > Visual Sidechain is an independent third-party plugin for OBS Studio. It is not made, endorsed, or supported by the OBS Project or NVIDIA Corporation.
- **`R-ID-4` (AI Coding Tools Disclosure):** `README.md` and release notes MUST include an honest disclosure that the project is developed with the assistance of AI coding tools under human direction, review, and hardware verification.

---

## 2. Licensing & Upstream Attribution (`R-LIC-*`)

- **`R-LIC-1` (Repository License):** The repository license is **GNU General Public License v2.0** (`LICENSE` at commit `cf2f276`). `LICENSE` MUST NOT be altered or replaced without explicit operator instruction.
- **`R-LIC-2` (Source Header Stance):** All original source files (`src/*.c`, `src/*.h`, `data/effects/*.effect`) authored in this project MUST carry an `SPDX-License-Identifier: GPL-2.0-or-later` header so our code is compatible both with `GPL-2.0-only` plugins and with OBS Studio's `GPL-2.0-or-later` headers.
- **`R-LIC-3` (All Shaders Original or GPL-Compatible — No Shadertoy Copies):**
  - Every `.effect` shader shipped in `data/effects/` MUST be 100% original code authored for Visual Sidechain under `GPL-2.0-or-later` (or explicitly verified GPL-2.0-compatible code with full attribution).
  - **No Shadertoy copies:** Do NOT copy, port, or vendor shaders from Shadertoy (whose default license is `CC BY-NC-SA 3.0`, which is GPL-incompatible) or from `exeldro/obs-shaderfilter`'s `data/examples/` directory.
- **`R-LIC-4` (Upstream Attribution):** Any code or patterns adapted from third-party open-source projects (`obs-studio` GPL-2.0-or-later, `mmlTools/audio-wave` MIT, `exeldro/obs-move-transition` GPL-2.0, `exeldro/obs-shaderfilter` GPL-2.0-only, or KissFFT BSD-3-Clause) MUST retain upstream copyright notices, license text, and attribution in `THIRD_PARTY_NOTICES.md` and relevant source headers.
- **`R-LIC-5` (No Unsolicited Upstream Contact):** Never contact `exeldro` or any other upstream author (no issues, PRs, emails, or messages) unless the operator explicitly instructs us to do so.

---

## 3. Target Platform & Build (`R-PLAT-*`)

- **`R-PLAT-1` (OBS Version):** Target **OBS Studio 33.0.0** (`libobs` API version 33) on **Windows 10/11 x64** (`MSVC` / Visual Studio 2022 Build Tools).
- **`R-PLAT-2` (GPU Target):** Primary hardware target is **NVIDIA RTX GPUs** on Windows Direct3D 11 (`d3d11`), using OBS `libobs` graphics (`gs_*`) HLSL-compatible `.effect` shaders so all filters run natively in the OBS GPU render pipeline with zero CPU frame readback.
- **`R-PLAT-3` (Standalone CMake + Shared Local Deps):** The build MUST use CMake `>= 3.28` (`CMAKE_C_STANDARD 17`) and share the local prebuilt dependency cache from `C:\Users\jeffr\Documents\obs-playlister\.deps` (`obs-studio-33.0.0` and `obs-deps-2025-08-23-x64`) in read-only mode without modifying `obs-playlister`.

---

## 4. Audio Tap & Routing (`R-TAP-*`)

- **`R-TAP-1` (Default Audio Input is Track 1 / Main Mix):**
  - Every FFT shader filter and the Audio Modulator MUST default its `"Audio Source"` setting to **Track 1 (Main Mix)** (`__MAIN_MIX__`), tapped via `obs_get_audio()` and `audio_output_connect(audio_t *, 0, NULL, raw_audio_callback, param)` on mixer index `0`.
  - Adding any Visual Sidechain filter to a video source MUST immediately react to Track 1 audio with zero manual setup required.
- **`R-TAP-2` (Selectable Named Audio Sources & Parent Source):**
  - In addition to **Track 1 (Main Mix)** (default), the `"Audio Source"` dropdown MUST offer:
    1. **Track 1 (Main Mix)** (`__MAIN_MIX__`, default)
    2. **Track 2..6** (`__TRACK_2__` .. `__TRACK_6__` via `audio_output_connect` mixer indices `1..5`)
    3. **Parent Source Audio** (`__PARENT__` — the video source to which the filter is attached, if it carries audio)
    4. **Any Named Audio-Capable OBS Source** (`OBS_SOURCE_AUDIO` enumerated via `obs_enum_sources`), tapped via `obs_source_add_audio_capture_callback`.
- **`R-TAP-3` (Shared Audio Hub):** If multiple filters tap the same audio source or Track 1..6, a reference-counted shared hub (`vsc_audio_hub`) MUST attach only a single OBS audio callback and run the FFT/DSP analyzer once per audio block, sharing the resulting spectrum and feature snapshot across all subscribed filters.
- **`R-TAP-4` (Weak Source References & Lifecycle Safety):** Named audio sources MUST be held via `obs_weak_source_t` (`obs_source_get_weak_source`), never raw `obs_source_t *` across ticks. If a tapped source is renamed, destroyed, or not yet created at scene-load time, the hub MUST lazily resolve the source by name when available and safely output zeroed audio features while missing.
- **`R-TAP-5` (Sample Rate & Channel Agnostic):** The tap MUST accept whatever sample rate (`44100 Hz`, `48000 Hz`, etc.) and channel count (`mono`, `stereo`, `surround`) OBS provides in float planar format (`AUDIO_FORMAT_FLOAT_PLANAR`), downmixing active channels to mono (and optionally tracking L/R stereo difference) before feeding the lock-free ring buffer.

---

## 5. Real-Time FFT & DSP Analyzer (`R-ANA-*`)

- **`R-ANA-1` (Zero-Allocation Audio Thread):** The OBS audio callback (`obs_source_audio_capture_t` or `audio_output_callback_t`) MUST be strictly real-time safe: no heap allocations (`malloc`/`bfree`), no blocking mutexes, no logging, and no `obs_source_update` calls on the audio thread.
- **`R-ANA-2` (High-Resolution Windowed FFT):**
  - Maintain a sliding 2048-sample Hann-windowed frame (`~42.7 ms` at `48 kHz`, hop size `512` samples or per-render-tick latest window) and compute a real-to-complex FFT yielding **1025 magnitude bins** (`~23.4 Hz` per bin at `48 kHz`).
  - Downsample/map the positive frequencies into:
    1. **512 normalized FFT bins (`fft_bins[512]`)**: mapped from dBFS (`-70 dBFS .. 0 dBFS`, configurable floor/ceiling) to `[0.0 .. 1.0]` with per-bin configurable asymmetric attack/release smoothing (`attack_ms`, `release_ms`).
    2. **32 logarithmic 1/3-octave spectrum bands (`bands[32]`)** and **32 peak-hold values (`band_peaks[32]`)** spanning `20 Hz` to `20 kHz`.
    3. **512 latest time-domain mono waveform samples (`waveform[512]`)** normalized to `[-1.0 .. 1.0]` (and `[0.0 .. 1.0]` for texture upload).
- **`R-ANA-3` (Arbitrary Custom Hz FFT Band Extractor):**
  - Every filter and modulator slot MUST allow the user to select either a preset audio feature OR a **Custom FFT Frequency Window (`freq_low_hz` .. `freq_high_hz`)** anywhere from `20 Hz` to `20000 Hz`.
  - Given `[freq_low_hz, freq_high_hz]`, the analyzer integrates the exact FFT bins `[k_low .. k_high]` using one of three selectable modes:
    - **Average Band Energy** (RMS of bins in range)
    - **Peak Bin Magnitude** (max bin in range, great for tracking a synth lead or fundamental note)
    - **Band Spectral Flux** (positive frame-to-frame magnitude increase in `[k_low .. k_high]`, ideal for transient/onset triggering in a specific frequency range).
- **`R-ANA-4` (Standard Musical Envelopes & Onset Features):**
  - Alongside the full 512-bin FFT, 32 log bands, and Custom Hz Window, compute:
    - `level` (wideband RMS `0.0..1.0`) and `peak` (wideband peak `0.0..1.0`)
    - `bass` (`20–250 Hz`), `sub_bass` (`20–60 Hz`), `mid` (`250–2000 Hz`), `high` (`2000–20000 Hz`)
    - `spectral_centroid` (normalized brightness center of mass `0.0..1.0`)
    - `spectral_flux` (full-spectrum positive onset flux `0.0..1.0`)
    - `kick` (`40–120 Hz` spectral-flux onset detector with adaptive moving-average threshold)
    - `snare` (`150–4000 Hz` transient + high-frequency noise burst detector)
    - `beat_phase` (`0.0..1.0` sawtooth phase locked to detected kick/onset tempo with decay fallback).
- **`R-ANA-5` (Silence & NaN/Inf Protection):** Below the noise gate threshold (`gate_db`, default `-65 dBFS`), all bins and envelopes decay smoothly to `0.0`. Every output float MUST be sanitized (`isfinite`) and clamped to `[0.0, 1.0]`.

---

## 6. GPU FFT Texture Pipeline & Built-in FFT-Reactive Shader Filters (`R-SHD-*`)

- **`R-SHD-1` (Shared GPU FFT Textures Bound to Every Shader):**
  - Once per video render tick, the shared hub uploads the latest analyzer snapshot into two dynamic GPU textures (`gs_texture_t`) shared by all filters reading that audio tap:
    1. **`vsc_fft_tex` (`512 x 2`, `GS_R32F`)**:
       - Row 0 (`v = 0.25`): `512` smoothed FFT magnitude bins (`0.0..1.0`).
       - Row 1 (`v = 0.75`): `512` time-domain waveform samples mapped to `0.0..1.0` (`0.5` = silence).
    2. **`vsc_bands_tex` (`32 x 1`, `GS_R32F`)**:
       - `32` logarithmic spectrum bands (`0.0..1.0`).
  - Every Visual Sidechain `.effect` shader receives `vsc_fft_tex`, `vsc_bands_tex`, the user's tuned `audio_react` scalar (computed from their selected preset or Custom Hz FFT window after gain/threshold/curve/attack/release), and the full set of `audio_*` uniforms (`audio_bass`, `audio_mid`, `audio_high`, `audio_kick`, `audio_snare`, `audio_beat_phase`, `audio_spectral_centroid`, `audio_spectral_flux`, `elapsed_time`, `uv_size`).
- **`R-SHD-2` (Built-in FFT-Reactive Filter Suite — 10 GPU Shader Filters):**
  The plugin MUST register the following 10 original GPU `.effect` filters (`OBS_SOURCE_TYPE_FILTER`, `OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW` or `obs_source_process_filter_begin/end`):
  1. **`vsc_filter_edge_aura` ("Visual Sidechain — FFT Spectral Aura / Edge Glow")**:
     - Detects the alpha silhouette (e.g., webcam cutout after **NVIDIA Background Removal**) or frame border using multi-tap alpha/luma gradient sampling, and maps perimeter angle `theta` or edge position directly into `vsc_fft_tex` / `vsc_bands_tex`. Low frequencies glow at the base/sides while mids and highs shimmer around the contour, with configurable aura width, color gradient, and bloom intensity.
  2. **`vsc_filter_displacement` ("Visual Sidechain — FFT Wave Warp & Glitch")**:
     - Spatial FFT UV distortion: samples `vsc_fft_tex` across screen coordinates so low FFT bins drive broad sinusoidal/liquid shear waves, mid bins drive fine ripple waves, and high-frequency spectral flux triggers horizontal scanline glitch tears and RGB block offsets.
  3. **`vsc_filter_prism` ("Visual Sidechain — FFT Chromatic Prism & Bloom")**:
     - Spectral RGB/prism dispersion where bass, mid, and high FFT energy (or custom Hz bins) independently drive radial/directional channel separation, spectral hue shift, and dual-tap glow/bloom above a brightness threshold.
  4. **`vsc_filter_visualizer` ("Visual Sidechain — FFT Visualizer Overlay")**:
     - Renders GPU-accelerated FFT visualizations directly from `vsc_fft_tex` and `vsc_bands_tex` composited over the source video (or over transparent/solid background):
       - **Mode 0: Spectrum Bars** (32 log bands or 128/512 FFT bins with rounded caps and peak-hold markers)
       - **Mode 1: Polar / Circular Spectrum Ring** (radial frequency bars wrapped around a center point or webcam)
       - **Mode 2: Spectrogram / Waterfall** (rolling 2D history texture `512 x 256` updated one row per frame showing frequency vs. time in heat/neon palettes)
       - **Mode 3: Oscilloscope Waveform** (anti-aliased glowing ribbon from row 1 of `vsc_fft_tex`).
  5. **`vsc_filter_zoom_pulse` ("Visual Sidechain — FFT Zoom Pulse")**:
     - Drives radial scale zoom, barrel/lens punch, and subtle edge blur from any FFT band / kick onset / custom Hz window around a configurable center point, with clamp-to-edge/mirror UV padding so borders never clip to black.
  6. **`vsc_filter_kaleidoscope` ("Visual Sidechain — FFT Kaleidoscope")**:
     - Polar N-fold mirror reflection (`2..24` segments) where FFT bass/kick drives radial zoom and segment pulse, mid/centroid drives continuous or beat-stepped rotation, and each polar angle samples `vsc_fft_tex` for per-ray spectral ripple.
  7. **`vsc_filter_feedback` ("Visual Sidechain — FFT Feedback Trails")**:
     - Uses ping-pong GPU render targets (`gs_texrender_t` `accum_a` / `accum_b`) to blend the current video frame with the transformed previous frame: FFT energy controls trail persistence/decay (`0.0..0.98`), radial expansion/zoom drift, slight rotation, and spectral hue shift on the echo trails.
  8. **`vsc_filter_strobe` ("Visual Sidechain — FFT Strobe & Flash")**:
     - FFT onset / custom-Hz band triggered strobe supporting **Brightness/Exposure Burst**, **Color Tint Flash**, **Invert Flash**, and **Rhythmic Black Gate**, equipped with a mandatory **Max Strobe Rate (Hz)** (`1..30 Hz`) and **Max Duty Cycle / Intensity** limiter.
  9. **`vsc_filter_mirror` ("Visual Sidechain — FFT Mirror & Split")**:
     - 2-way horizontal/vertical mirror, 4-way quad mirror, diagonal mirror, and multi-slice split bounce where FFT bins offset slice positions or flip mirror axes on beat onsets.
  10. **`vsc_filter_custom_shader` ("Visual Sidechain — Custom FFT Shader")**:
      - Loads a user-selected `.effect` shader file, exposes its declared `uniform` parameters in the Filter Properties UI, and automatically binds `vsc_fft_tex`, `vsc_bands_tex`, `vsc_prev_frame_tex` (optional ping-pong feedback texture), and all `audio_*` uniforms.
- **`R-SHD-3` (Common Per-Filter FFT Controls):**
  - Every built-in filter MUST expose a consistent **"FFT & Audio Reactivity"** control group:
    - `audio_source` (default: **Track 1 (Main Mix)**)
    - `fft_driver` (`Custom Hz Band`, `Sub-Bass 20–60 Hz`, `Bass 60–250 Hz`, `Mid 250–2000 Hz`, `High 2–20 kHz`, `Kick Onset`, `Snare Onset`, `Spectral Flux`, `Spectral Centroid`, `Beat Phase`, `Full RMS`)
    - `freq_low_hz` (`20..20000 Hz`) and `freq_high_hz` (`20..20000 Hz`) + `band_mode` (`Average`, `Peak Bin`, `Spectral Flux`)
    - `gain` (`0.0..10.0`), `threshold` (`0.0..1.0`), `attack_ms` (`1..500 ms`), `release_ms` (`10..2000 ms`), `curve` (`Linear`, `Exponential`, `Logarithmic`, `S-Curve`, `Gate`), `amount` (`-2.0..2.0`), `mix` (`0.0..1.0`).
- **`R-SHD-4` (Graphics Context & Render Target Hygiene):**
  - All `gs_effect_create_from_file`, `gs_texrender_create`, `gs_texture_create`, and `gs_effect_destroy` calls MUST occur inside `obs_enter_graphics()` / `obs_leave_graphics()`.
  - Ping-pong `gs_texrender_t` buffers (`vsc_filter_feedback`, `vsc_filter_visualizer` waterfall, `vsc_filter_custom_shader`) MUST resize automatically when the parent source's `cx`/`cy` changes and free cleanly in `filter_destroy`.
  - If `cx == 0 || cy == 0` or an effect fails to compile, the filter MUST fall back to `obs_source_skip_video_filter(context)` without crashing.

---

## 7. Sibling-Filter Audio Modulator & NVIDIA RTX Integration (`R-MOD-*`, `R-NV-*`)

- **`R-MOD-1` (Keep Generic Modulator for Sibling Filters):**
  - Register **`vsc_modulator` ("Visual Sidechain — Audio Modulator")**, a pass-through video filter (`obs_source_skip_video_filter`) that enumerates sibling filters on the same parent source (`obs_source_enum_filters`), inspects their `obs_properties_t` (`obs_source_properties`), and drives up to **8 modulation slots** per instance.
  - Default audio input is **Track 1 (Main Mix)**.
- **`R-MOD-2` (Mapping Math & FFT Custom Hz Support):**
  - Each slot selects:
    - Target sibling filter (by name) + target numeric property (`OBS_PROPERTY_FLOAT` or `OBS_PROPERTY_INT`)
    - Audio driver: any standard feature OR a **Custom FFT Hz Range (`freq_low_hz .. freq_high_hz`)** (`Average`, `Peak Bin`, `Spectral Flux`)
    - `gain`, `threshold`, `attack_ms`, `release_ms`, `curve`, `invert`, `output_min`, and `output_max`.
- **`R-MOD-3` (Batched Updates, Epsilon Gate, and Rate Divider for NVIDIA RTX Filters):**
  - **First-class compatibility with OBS 33's `nv-filters` (`nvidia-videofx-filter.c`)**:
    1. **NVIDIA Blur (`nv_blur_filter`)** — `intensity` (`0.0..1.0`)
    2. **NVIDIA Background Blur (`nv_background_blur_filter`)** — `intensity` (`0.0..1.0`)
    3. **NVIDIA Background Removal (`nv_greenscreen_filter`)** — `threshold` (`0.0..1.0`) and `processing_interval` (`1..4`).
  - Because `nvvfx_filter_update` in OBS 33 calls `NvVFX_SetF32` + `NvVFX_Load` when `intensity` changes, `vsc_modulator` MUST:
    - Batch all active slots targeting the same sibling filter into a single `obs_source_update` call per tick.
    - Gate updates with an epsilon check (`|new_val - last_sent_val| >= epsilon`, default `0.002`) so idle or steady audio makes **zero** `obs_source_update` calls.
    - Provide an **Update Rate Divider** (`Every Frame`, `Every 2nd Frame`, `Every 4th Frame`) per slot/target so heavy SDK filters like `NvVFX_Load` stay smooth at 60 fps.
- **`R-MOD-4` (Baseline Restoration):**
  - When a slot is disabled, retargeted, or the Audio Modulator filter is disabled/removed, restore the target property's baseline value captured before modulation began.

---

## 8. Threading, Determinism, Testing & Working Rules (`R-THR-*`, `R-TST-*`, `R-STD-*`)

- **`R-THR-1` (Three-Thread Contract):**
  - **Audio Thread:** Lock-free SPSC ring buffer push only.
  - **Video Render Thread (`video_tick` / `video_render`):** Drains ring buffer, computes FFT & envelopes, updates `vsc_fft_tex` (`512x2`) and `vsc_bands_tex` (`32x1`), renders `.effect` shaders, and applies batched `obs_source_update` calls for `vsc_modulator`.
  - **UI Thread:** Reads atomic/mutex-guarded config and enumerates sources/properties only inside `get_properties` / property callbacks.
- **`R-TST-1` (Pure-C Headless Test Suite via `ctest`):**
  - Every phase MUST ship with deterministic unit tests in `tests/` that run without an OBS instance or GPU:
    - `test_ringbuf`: SPSC ring buffer wrap-around, overflow policy, planar downmix.
    - `test_analyzer`: Hann window, 2048-point FFT bin accuracy (`60 Hz`, `440 Hz`, `1000 Hz`, `10000 Hz` sine waves), 512-bin dBFS normalization, 32 log bands, **Custom Hz Window (`freq_low_hz..freq_high_hz`)** extraction (`Average`, `Peak Bin`, `Spectral Flux`), kick/snare onset detection, silence decay, NaN/Inf immunity.
    - `test_mapping`: Envelope follower (`attack_ms`/`release_ms`), shaping curves, range clamping, epsilon gate, rate divider.
    - `test_effect_parser`: Custom `.effect` uniform parser for `vsc_filter_custom_shader` and headless syntax/uniform verification of all 10 shipped `.effect` files.
- **`R-STD-1` (Formatting & Hygiene):**
  - Run `clang-format -i` on all `.c`/`.h` files before every commit.
  - Every file MUST be UTF-8 **without** BOM, non-zero bytes, with zero occurrences of the banned placeholder word (`d-u-m-m-y`; use `placeholder` or `stub`) and zero profanity.
  - Do not touch `C:\Users\jeffr\Documents\obs-playlister`.
  - Maximum one `git push` per commit cycle, followed by `HEAD == origin/main`, 0-byte, and BOM verification.

---

## 9. Resolved Operator Decisions (`D-1..D-4`)

| ID | Decision | Resolution |
|---|---|---|
| **`D-1`** | **Shader Strategy** | **100% original GPL-2.0-or-later `.effect` shaders** (no Shadertoy copies, no `obs-shaderfilter` `data/examples/`), with `vsc_fft_tex` (`512x2`) + `vsc_bands_tex` (`32x1`) bound to every shader, plus a **Custom FFT Shader** filter that loads user `.effect` files. |
| **`D-2`** | **Default Audio Input & Modulator** | Default audio input is **Track 1 (Main Mix)** (`obs_get_audio()`). Keep **Audio Modulator** (`vsc_modulator`) for driving sibling filters including OBS's **NVIDIA Blur**, **Background Blur**, and **Background Removal**. |
| **`D-3`** | **FFT-Reactive Filter Roster** | Ship all **10 FFT-reactive GPU shader filters** (Spectral Aura/Edge Glow, Wave Warp & Glitch, Chromatic Prism & Bloom, Visualizer Overlay, Zoom Pulse, Kaleidoscope, Feedback Trails, Strobe & Flash, Mirror & Split, Custom FFT Shader) + **Audio Modulator** (11 registered filters total). |
| **`D-4`** | **FFT Engine** | Self-contained, dependency-free radix-2 Cooley-Tukey 2048-point real FFT in `src/vsc-fft.c` (`GPL-2.0-or-later`) with `512`-bin texture output, `32` log bands, and arbitrary `[freq_low_hz .. freq_high_hz]` band integration. |
