# visual-sidechain -- Testing & Hardware Verification Results

Companion to [REQUIREMENTS.md](REQUIREMENTS.md) and [PLAN.md](PLAN.md).

---

## Phase 1 -- OBS 33 Build, Layout, and Load Smoke Checks (`0.1.0`)

| # | Check | Measured Log Line / Result | Status |
|---|-------|----------------------------|--------|
| 1.1 | Configure & build `visual-sidechain` (`cmake --preset windows-x64` && `cmake --build --preset windows-x64 --config RelWithDebInfo`) | Compiles with 0 warnings (`CMAKE_COMPILE_WARNING_AS_ERROR` ON); `dumpbin /exports` shows `obs_module_load`, `obs_module_unload`, `obs_module_name`, `obs_module_description`. | Passed (2026-10-27) |
| 1.2 | CTest suite (`ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error`) and mutation check | `check-locale-keys` and `check-locale-keys-fails-on-missing-key` pass (`2/2`); deliberate `MissingKeyProbe` in `src/plugin-main.c` causes `check-locale-keys` to fail non-zero (`exit code 8`). | Passed (2026-10-27) |
| 1.3 | Install layout (`cmake --install build_x64 --config RelWithDebInfo --prefix release/RelWithDebInfo`) | Produces `release/RelWithDebInfo/visual-sidechain/visual-sidechain.dll`, `visual-sidechain.pdb`, `data/manifest.json`, `data/locale/en-US.ini`, and `data/shaders/*.shader` (no `bin/64bit`). | Passed (2026-10-27) |
| 1.4 | Portable OBS 33.0.0 load & clean exit (`C:\Users\jeffr\OBS-Studio-33-Portable`) | Copy to `plugins\visual-sidechain\`, launch from `bin\64bit`, close with `CloseMainWindow()`: log `2026-10-27 13-24-25.txt` contains `[visual-sidechain] plugin loaded successfully`, `[visual-sidechain] plugin unloaded`, and `Number of memory leaks: 0`; `plugin_manager/modules.json` records `"display_name": "Visual Sidechain"` with no `Legacy` flag. | Passed (2026-10-27) |
| 1.5 | Windows security & unsigned DLL (`R-PLAT-10`) | `Get-AuthenticodeSignature` reports `NotSigned` as expected; `visual-sidechain.dll` loads cleanly in OBS 33.0.0 without Defender or Code Integrity blocks. | Passed (2026-10-27) |

---

## Phase 2 -- Dual-FFT Analysis Engine, WAV Fixtures, and Audio Tap (`0.2.0`)

| # | Check | Measured Log Line / Result | Status |
|---|-------|----------------------------|--------|
| 2.1 | `test-audio-ring` (`R-ANA-2`, `R-ANA-3`, `R-TAP-5`) | Verifies 32,768-sample ring wraparound, stereo planar downmix + `NaN`/`Inf` sanitization, mute vs ghost-sidechain (`react_when_muted`), and `+100 ms` (`4800`-sample) `sync_offset_ms` read-cursor delay. Mutation check (`+1` sample offset) failed at `tests/test-audio-ring.c:94` (`exit code 8`). | Passed (2026-10-27) |
| 2.2 | `test-fft` (`R-ANA-4`) | Verifies dual-resolution Hann-windowed Cooley-Tukey FFT (`N = 4096` -> `11.72 Hz` bin spacing at 48 kHz; `N = 2048` -> `23.44 Hz` spacing) and confirms `35 Hz` (`k = 3`) vs `60 Hz` (`k = 5`) separation in the 4096-pt FFT (`valley_bin4 = 0.043 * peak_35`). Mutation check (bit-reversal swap commented out) failed at `tests/test-fft.c:49` (`exit code 8`). | Passed (2026-10-27) |
| 2.3 | `test-spectrum` (`R-ANA-5`) | Verifies 32 log-spaced bands (`spectrum[32]`), 512 log-frequency bins (`fft_bins[512]`), 512 waveform samples (`waveform[512]`), and `vs_spectrum_extract_hz_band(70..110 Hz)`. Mutation check (routing low bands `<= 250 Hz` through 2048-pt FFT) failed at `tests/test-spectrum.c:64` (`exit code 8`). | Passed (2026-10-27) |
| 2.4 | `test-envelope` (`R-ANA-6`, `R-ANA-7`) | Verifies `-55` dBFS noise gate (`-70` dBFS noise outputs `0.0f`), auto-gain convergence across `-30` dBFS and `-6` dBFS, asymmetric `attack_ms` (`100 ms` -> $1 - e^{-1} = 0.6321$) / `release_ms` (`200 ms` -> $e^{-1} = 0.3679$) smoothing, and starvation decay. Mutation check (bypassing `gate_db`) failed at `tests/test-envelope.c:43` (`exit code 8`). | Passed (2026-10-27) |
| 2.5 | `test-onset` & `test-analyzer` on 48 kHz WAV fixtures (`R-ANA-8..10`, `R-TST-2`) | Runs headless against `tests/fixtures/{kick-120bpm-4bars,subbass-35hz-vs-kick-95hz,dynamic-range-40db,silence-and-dc}.wav`: detects all 8 kicks (`kick > 0.7`), all 4 snares on beats 2 & 4 (`snare > 0.7`, `< 0.15` on kick-only beats), locks tempo to `120.0 BPM`, separates `95 Hz` kicks on top of a `35 Hz` sub-bass drone (`sub_bass > 0.65`, `kick > 0.65`), and removes steady `+0.25` DC offset (`level == 0.0`). Mutation check (`700 ms` refractory cooldown) failed `test-onset:109` and `test-analyzer:72` (`exit code 8`). | Passed (2026-10-27) |
| 2.6 | Portable OBS 33.0.0 Audio Tap attachment (`R-TAP-1..6`) | Verified in log `2026-10-27 14-22-20.txt`: `[visual-sidechain] Attached audio tap to Track 1 (mix_idx=0)` (default), `[visual-sidechain] Attached audio tap to named source 'KickTrack'`, `[visual-sidechain] Attached audio tap to parent source 'KickTrack'`, and `Number of memory leaks: 0` on shutdown. | Passed (2026-10-27) |

---

## Phase 3 -- Audio Modulator, Baseline Restoration, NVIDIA RTX & Sync Lag (`0.3.0`)

| # | Check | Measured Log Line / Result | Status |
|---|-------|----------------------------|--------|
| 3.1 | `test-modulation` & `test-modulator-baseline` (`R-MOD-3..6`) | Verifies positive/ducking slot math, integer step quantization, rate divider (`1/2/4`), epsilon gate (`1e-4`), one-time baseline capture (`0.25` never overwritten by `0.85` pulse), restoration on disable/retarget/remove, pre-save restoration (`vs_baseline_on_pre_save`), and crash-recovery reload (`vs_baseline_on_load`). Mutation check (skipping `apply_cb` in `vs_baseline_on_pre_save`) failed at `tests/test-modulator-baseline.c:114` (`exit code 8`). | Passed (2026-10-27) |
| 3.2 | Portable OBS 33.0.0 Scene-Save Baseline Protection (`R-MOD-5`, `R-MOD-6`) | In portable OBS 33.0.0 (`2026-10-27 14-19-44.txt`), `Audio Modulator` pulsed `Color Correction` (`color_filter_v2`) `saturation` between `0.25` and `0.95` on `KickTrack`. Closing OBS via `CloseMainWindow()` invoked `modulator_filter_save` (`vs_baseline_on_pre_save`), and inspecting `Untitled.json` on disk confirmed `saturation: 0.25` (exact baseline) with `Number of memory leaks: 0`. | Passed (2026-10-27) |
| 3.3 | NVIDIA RTX Filter Compatibility & Hardware Finding (`R-NV-1`, `R-NV-2`) | In portable OBS 33.0.0 (`2026-10-27 14-20-47.txt`), `Audio Modulator` bound to `nv_greenscreen_filter` (`threshold = 0.50`), `nv_blur_filter` (`intensity = 0.30`), and `nv_background_blur_filter` (`intensity = 0.40`), preserving all three baselines (`0.50`, `0.30`, `0.40`) in `Untitled.json`. Hardware finding on RTX 5080 Laptop GPU (Blackwell SM 12.0): NVIDIA Video FX redistributable `0.7.6.0` logs `[NVIDIA Video Effect: '[E] Missing src dst']` during `NvVFX_Load` on SM 12.0 when `intensity` or `mode` triggers `NvVFX_Load`, whereas `threshold` and `processing_interval` on `nv_greenscreen_filter` are simple struct assignments (`filter->threshold`, `filter->processing_interval`) and do not call `NvVFX_Load`. Use rate divider `Every 2 Frames` or `Every 4 Frames` when modulating `intensity`. | Passed (2026-10-27) |
| 3.4 | Audio-to-Picture Sync Offset Verification (`R-ANA-3`) | Note: `+42.7 ms` is the 4096-point low-band Hann FFT analysis window center (half of the `85.33 ms` window at 48 kHz), **not** a measured audio-to-picture lag. Read-cursor delay math is verified headlessly in `test-audio-ring` (`+100 ms` = `4800`-sample exact delay), and slider operation (`-200..+500 ms` across the `682.7 ms` ring buffer) was smoke-checked in portable OBS 33.0.0 (`2026-10-27 14-22-20.txt`); end-to-end flash-and-click frame delta measurement in OBS is pending operator measurement. | Pending: operator measurement |

---

## Phase 4 -- Shader Filter Host & 9 Original FFT-Reactive Shaders (`0.4.0`)

| # | Check | Measured Log Line / Result | Status |
|---|-------|----------------------------|--------|
| 4.1 | `test-shader-parser` (`R-UP-3`, `R-UP-4`, `R-LIC-5/6`) | Verifies cyclic `#include` recursion cap at depth 16 (`VS_SHADER_ERR_INCLUDE_DEPTH`), malformed quote rejection (`VS_SHADER_ERR_MALFORMED_INCLUDE`), uniform `< ... >` annotation extraction (`float`, `int`, `bool`, `float4`), and validates all 9 built-in `.shader` files in `data/shaders/` for `GPL-2.0-or-later`, balanced braces, `audio_*`/`vs_sample_*` reactivity, zero BOMs, and zero Shadertoy / `CC BY-NC-SA` provenance. Mutation checks on depth guard (`line 37`) and brace balance (`line 187`) both failed non-zero (`exit code 8`). | Passed (2026-10-27) |
| 4.2 | Portable OBS 33.0.0 D3D11 Compilation of All 9 Built-In Shaders (`R-SHD-1..4`) | Verified in log `2026-10-27 14-19-44.txt` on NVIDIA GeForce RTX 5080 Laptop GPU (D3D11): all 9 shaders (`fft-spectral-aura.shader`, `fft-wave-warp-glitch.shader`, `fft-chromatic-prism-bloom.shader`, `fft-visualizer-overlay.shader`, `fft-zoom-pulse.shader`, `fft-kaleidoscope.shader`, `fft-feedback-trails.shader`, `fft-strobe-flash.shader`, `fft-mirror-split.shader`) compiled with 0 HLSL warnings/errors. | Passed (2026-10-27) |
| 4.3 | Stacked GPU Frame-Time Benchmark on RTX 5080 (`TESTING.md` 4.3) | With **all 9 FFT shaders + Audio Modulator active simultaneously on a 1920x1080 source** in OBS 33.0.0 (`2026-10-27 14-19-44.txt`), OBS Profiler recorded: `tick_sources: min=0.001 ms, median=0.729 ms, 99th percentile=2.351 ms`; `render_video: min=0.005 ms, median=0.073 ms, max=1.186 ms, 99th percentile=0.326 ms` (well below the `16.67 ms` 60 fps budget), and `Number of memory leaks: 0` on exit. | Passed (2026-10-27) |

---

## Phase 5 -- Vendor API Signal Publishing & Fader Slots as Modulation Sources (`0.5.0`)

| # | Check | Measured Log Line / Result | Status |
|---|-------|----------------------------|--------|
| 5.1 | `test-fader-bridge` (`R-MOD-7`, `R-VND-1..3`) | Verifies all 10 Fadester slot IDs (`crossfader`, `fader_1`..`fader_8`, `master`), `[0.0, 1.0]` clamping, missing `midi_fadester_get_slot_value` proc reporting `!vs_fader_bridge_is_midi_fadester_loaded()` and reading `0.0f` across all 10 fader slots, fake `midi_fadester_get_slot_value` proc polled at most once per tick + `midi_fadester_fader_changed` signal updating slots immediately, and `visual_sidechain_get_signals` proc + 30 Hz `visual_sidechain_signals` signal emitting `level, bass, mid, high, kick, snare, beat_phase, bpm, ok`. | Passed (2026-10-27) |
| 5.2 | Portable OBS 33.0.0 `obs-websocket` Vendor API & In-Process Bridge Registration (`R-VND-1..3`, `R-MOD-7`) | Verified in portable OBS 33.0.0: `[visual-sidechain] plugin loaded successfully (version 0.5.0)`, `[visual-sidechain] Registered obs-websocket vendor 'visual-sidechain' (get_signals=1)` (no `get_faders` or `set_fader_value` on `visual-sidechain`; Midi Fadester owns faders), `[visual-sidechain] plugin unloaded`, and `Number of memory leaks: 0`. | Passed (2026-10-27) |
| 5.3 | Live bridge: both plugins loaded in portable OBS, a Midi Fadester fader move arrives as a fader source value | Verified in portable OBS 33.0.0 (`2026-10-27 15-07-19.txt`) with `midi-fadester.dll` (`208d94f`) and `visual-sidechain.dll` (`bbd6e38`) both loaded: `[midi-fadester] Registered in-process bridge procs (midi_fadester_get_slot_value, midi_fadester_set_fader_value) and signal (midi_fadester_fader_changed)`, `[visual-sidechain] Registered obs-websocket vendor 'visual-sidechain' (get_signals=1)`, `[bridge-test] midi_fadester_set_fader_value(crossfader, 0.75) called=true ok=true`, `[bridge-test] Modulator UI slot_1_signal[200] label='Fader: Crossfader'` (no `"Midi Fadester is not loaded"` suffix), `[bridge-test] Color Correction modulated opacity=0.7500 (expected 0.7500)`, and `Number of memory leaks: 0`. | Passed (2026-10-27) |
