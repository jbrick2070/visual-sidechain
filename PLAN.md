# visual-sidechain -- Plan

Status: 2026-10-27. Updated for operator review (Phase 0: plan only, no code).
Companion to [REQUIREMENTS.md](REQUIREMENTS.md); requirement IDs (`R-...`) and
decisions (`D-...`) refer to that file.

---

## 1. Toolchain and workspace findings on this machine (checked 2026-10-27)

| Component | Found | Needed | Action |
|-----------|-------|--------|--------|
| Workspace folder | `C:\visual-sidechain` (created and opened by the operator) | Canonical repo path (`R-ID-2`, `D-1`) | Keep at `C:\visual-sidechain` permanently; do not move later. |
| Visual Studio | Build Tools 2022, 17.14 (`C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`), MSVC 14.44, Windows 11 SDK `10.0.26100.0` | VS 2022 C/C++ toolchain + SDK 26100 | None. Generator `Visual Studio 17 2022` with `"architecture": "x64,version=10.0.26100.0"`, proven in `obs-playlister`. |
| CMake | 4.4.0 (`C:\Program Files\CMake\bin`) | 3.28..3.30 | None. |
| git / gh | git 2.56.0.windows.3 (`core.autocrlf=true`); `gh` 2.89.0 signed in as `jbrick2070` | authenticated | None. |
| clang-format | 19.1.5 at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\x64\bin\clang-format.exe` | Matches CI's `clang-format` 17 on `.clang-format` rules | Run this binary on every C/H file before committing (`R-STD-4`). |
| gersemi | Not installed in local Python 3.10 environment | CI runs `gersemi` 0.21.0 and fails on any output line | Keep `CMakeLists.txt` and `tests/CMakeLists.txt` strictly aligned with `obs-playlister`'s gersemi-clean structure (2-space indent, 120 columns, no `# cmake-format` fences). |
| Inno Setup | Not installed locally | `.exe` installer | CI (`windows-2022`) builds the `.exe` installer; local builds produce the system and portable `.zip` trees. |
| Portable OBS 33.0.0 | `C:\Users\jeffr\OBS-Studio-33-Portable` (`bin\64bit\obs64.exe` verified present) | Smoke and integration testing | Launch with `-WorkingDirectory "C:\Users\jeffr\OBS-Studio-33-Portable\bin\64bit"`, close with `$proc.CloseMainWindow()` (`R-PLAT-5`). |
| Cached OBS 33 deps | `C:\Users\jeffr\Documents\obs-playlister\.deps` holds `33.0.0.zip`, `windows-deps-2026-10-23-x64.zip`, `windows-deps-qt6-2026-10-23-x64.zip` | Avoid redundant multi-hundred-MB downloads if desired | `buildspec.json` uses the identical versions and SHA-256 hashes (`R-PLAT-7`). |
| GPU | NVIDIA RTX 5080 Laptop GPU | NVIDIA RTX target (`R-PLAT-1`, `R-NV-1`) | Used for shader and `nv-filters` modulation benchmarks. |

---

## 2. OBS 33 and upstream facts that drive the design (verified against sources)

1. **Audio taps in `libobs/obs.h` (tag `33.0.0`)**:
   - **Track 1..6 mix tap (`obs.h:932-934`, `obs.c:3153-3164`, `media-io/audio-io.c:289-315`)**: `obs_add_raw_audio_callback(size_t mix_idx, const struct audio_convert_info *conversion, audio_output_callback_t callback, void *param)` and `obs_remove_raw_audio_callback(mix_idx, callback, param)`. `mix_idx = 0` is **Track 1 (the main stream/recording mix)**, which is the default on every Visual Sidechain filter (`R-TAP-1`). Passing `conversion` with `format = AUDIO_FORMAT_FLOAT_PLANAR` (or `NULL` to inherit OBS's float planar mix format) delivers planar `float *` channel buffers (`data->data[ch]`, `data->frames`, `data->timestamp`) on the OBS audio output thread.
   - **Per-source audio tap (`obs.h:1347-1355`, `obs-source.c:5572-5594`)**: `obs_source_add_audio_capture_callback(obs_source_t *source, obs_source_audio_capture_t callback, void *param)` and `obs_source_remove_audio_capture_callback(...)`. Invoked from `source_output_audio_data` -> `source_signal_audio_data` (`obs-source.c:1522-1532, 1696`) after the source's audio has been resampled to OBS's float planar format and passed through the source's audio filters (`obs-source.c:4085-4100`). Passes `bool muted` (`obs-source.c:1594-1607`).
   - **Filtered source's own audio (`obs.h:1167`)**: `obs_filter_get_parent(filter)` returns the `obs_source_t *` to which the filter is attached; if `(obs_source_get_output_flags(parent) & OBS_SOURCE_AUDIO) != 0`, the same `obs_source_add_audio_capture_callback` attaches to `parent`.
2. **Driving another filter's settings in OBS 33 (`libobs/obs-source.c`) and scene-save implications**:
   - Calling `obs_source_update(target_filter, NULL)` after mutating `obs_data_t *ss = obs_source_get_settings(target_filter)` checks `target_filter->info.output_flags & OBS_SOURCE_VIDEO` (`obs-source.c:1112-1114`): for a video filter it increments `target_filter->defer_update_count`, and `obs_source_video_tick` runs `obs_source_deferred_update` (`target_filter->info.update`) on the graphics thread (`obs-source.c:1093-1101, 1410-1411`).
   - Because `obs_source_get_settings(target_filter)` is the live `obs_data_t *` that OBS serializes when saving a scene collection, modulating a setting without restoring its baseline on disable, removal, exit, or scene save would bake a mid-pulse value (e.g. `intensity = 0.80`) into the user's saved scene collection. `R-MOD-5` and `R-MOD-6` solve this with a dedicated baseline state machine (`src/core/baseline.c`) and pre-save/shutdown restoration.
3. **NVIDIA RTX Video Effects filters in OBS 33 (`plugins/nv-filters/nvidia-videofx-filter.c` at tag `33.0.0`)**:
   - Three filters registered in `nvidia-videofx-filter.c:1189-1235`:
     - `nv_greenscreen_filter` (**NVIDIA Background Removal**): `threshold` (`OBS_PROPERTY_FLOAT` `0.0..1.0`, step `0.05`, line 1064), `mode` (`OBS_PROPERTY_LIST` int `0..3`, lines 1058-1062), `processing_interval` (`OBS_PROPERTY_INT` `1..4`, step `1`, line 1065). Also outputs an alpha-masked silhouette that pairs directly with our `fft-spectral-aura.shader` (`R-SHD-3`).
     - `nv_blur_filter` (**NVIDIA Blur Filter**): `intensity` (`OBS_PROPERTY_FLOAT` `0.0..1.0`, step `0.05`, line 1055).
     - `nv_background_blur_filter` (**NVIDIA Background Blur Filter**): `intensity` (`OBS_PROPERTY_FLOAT` `0.0..1.0`, step `0.05`, line 1055).
   - **Critical finding in `nvvfx_filter_update` (`nvidia-videofx-filter.c:125-151`)**:
     - Updating `threshold` or `processing_interval` on `nv_greenscreen_filter` only writes `filter->threshold` / `filter->processing_interval` (lines 134, 150) and is read in `draw_greenscreen_blur` (line 831) — **near-zero CPU/GPU overhead per frame**.
     - Updating `intensity` on `nv_blur_filter` or `nv_background_blur_filter` when `filter->strength != strength` calls `NvVFX_SetF32(filter->handle_blur, NVVFX_STRENGTH, filter->strength)` followed by **`NvVFX_Load(filter->handle_blur)`** (lines 145-149).
     - Updating `mode` on `nv_greenscreen_filter` when `filter->mode != mode` calls `NvVFX_SetU32` followed by **`NvVFX_Load(filter->handle)`** (lines 136-142).
     - This confirms the project brief's first risk: driving `obs_source_update` every frame on filters that call `NvVFX_Load` inside `update` can stall the graphics thread unless guarded by a change-only epsilon check, step quantization, and an optional frame-rate divider (`R-MOD-4`, `R-NV-2`).
4. **Dual-resolution FFT requirement for sub-bass & kick (`R-ANA-4`)**:
   - At `48 kHz`, a single `2048`-point FFT has `48000 / 2048 = 23.4375 Hz` bin spacing, which puts only two bins inside the sub-bass band (`20..60 Hz`: bin 1 at `23.4 Hz` and bin 2 at `46.9 Hz`) and only four bins across `40..130 Hz` for kick detection.
   - Running a **4096-point Hann-windowed FFT** (`11.71875 Hz` bin spacing at `48 kHz`) for frequencies $\le 250\text{ Hz}$ doubles the bass frequency resolution (~4 bins across `20..60 Hz` and ~8 bins across `40..130 Hz`) while keeping the **2048-point FFT** (`42.7 ms` window) for frequencies $> 250\text{ Hz}$ so mid/high transients (`snare`, hi-hats) stay sharp in time.
5. **License header check of `exeldro/obs-shaderfilter` (`73fb5c2`, v2.6.1, 2026-10-24)**:
   - Recorded in detail in `REQUIREMENTS.md` `R-LIC-2` and `R-LIC-6`: `LICENSE` is GPL-2.0; `obs-shaderfilter.c` has no "or later" clause; 6 example shaders state `GPLv2` only; and 12+ example shaders in `data/examples/` are `CC BY-NC-SA 3.0` or Shadertoy ports. Any fork stays `GPL-2.0-only`, preserves nleseul / Charles Fettinger / Exeldro notices, and excludes `data/examples/`.

---

## 3. Target architecture (three layers)

```
src/
  plugin-main.c                  module entry; exports obs_module_name/description;
                                 initializes shared audio tap registry; registers filters
  plugin-support.c.in / .h       template logging ([visual-sidechain] prefix)
  core/                          Layer 2 + pure modulation/baseline math (C17, libobs util only,
                                 zero OBS source/graphics calls, 100% headless ctest)
    audio-ring.h / .c            circular mono float buffer (32,768 samples) + sync offset cursor
    fft.h / .c                   precomputed Hann windows + radix-2 complex FFT (N=4096 low + N=2048 mid/high)
    spectrum.h / .c              dual-FFT bins -> 32 log bands + 512 FFT texture bins + custom Hz band extractor
    envelope.h / .c              per-band auto-gain (with noise-gate floor) + attack/release smoothing
    onset.h / .c                 4096-pt kick & 2048-pt snare spectral-flux onset detectors + 60..180 BPM beat phase
    analyzer.h / .c              combines ring + dual FFT + spectrum + envelope + onset into vs_audio_frame
    modulation.h / .c            bipolar slot math, range mapping, int rounding, epsilon change gate, rate divider
    baseline.h / .c              per-(target, setting) baseline capture, restore on disable/remove/save/crash-reload
  obs/                           Layer 1 + Layer 3 OBS integration
    audio-tap.h / .c             Layer 1: shared/ref-counted taps for Track 1..6 (default Track 1),
                                 named source, and parent source; runs vs_analyzer once per video frame
                                 and uploads shared 32x1 audio_spectrum + 512x2 audio_fft GPU textures
    modulator-filter.h / .c      Layer 3b: Visual Sidechain: Audio Modulator filter (up to 8 slots,
                                 sibling filter & property introspection, batched obs_source_update,
                                 baseline restoration on disable/retarget/remove/save/unload)
    shader-filter.h / .c         Layer 3a: Visual Sidechain: Shader Filter (forked from obs-shaderfilter.c
                                 per D-2a or lean host per D-2b; binds audio_* uniforms + 32x1 & 512x2 FFT textures)
data/
  locale/en-US.ini               all UI strings (verified by check-locale-keys.cmake)
  internal/render_output.effect  pass-through output effect
  shaders/                       9 original GPL-2.0-or-later FFT-reactive shaders (R-SHD-3, zero Shadertoy copies)
    fft-spectral-aura.shader
    fft-wave-warp-glitch.shader
    fft-chromatic-prism-bloom.shader
    fft-visualizer-overlay.shader
    fft-zoom-pulse.shader
    fft-kaleidoscope.shader
    fft-feedback-trails.shader
    fft-strobe-flash.shader
    fft-mirror-split.shader
tests/
  CMakeLists.txt                 headless CTest runner (DLL PATH from .deps, no copying)
  check-locale-keys.cmake        verifies every obs_module_text/T_ key exists in en-US.ini
  wav-reader.h                   minimal RIFF/WAVE PCM/float reader for recorded WAV fixtures
  fixtures/                      recorded/rendered 48 kHz WAV test fixtures with known kicks & sub-bass
    kick-120bpm-4bars.wav
    subbass-35hz-vs-kick-95hz.wav
    dynamic-range-40db.wav
    silence-and-dc.wav
  test-audio-ring.c
  test-fft.c
  test-spectrum.c
  test-envelope.c
  test-onset.c
  test-analyzer.c
  test-modulation.c
  test-modulator-baseline.c
```

### 3.1 Core data contracts (`src/core/analyzer.h`, `src/core/modulation.h`, `src/core/baseline.h`)

```c
#define VS_SPECTRUM_BANDS 32
#define VS_FFT_TEX_BINS 512
#define VS_AUDIO_RING_CAPACITY 32768 /* ~682 ms at 48 kHz: covers 4096-pt FFT + +500 ms sync offset */

enum vs_signal_id {
	VS_SIG_LEVEL = 0,
	VS_SIG_SUB_BASS = 1,
	VS_SIG_BASS = 2,
	VS_SIG_MID = 3,
	VS_SIG_HIGH = 4,
	VS_SIG_KICK = 5,
	VS_SIG_SNARE = 6,
	VS_SIG_BEAT = 7,
	VS_SIG_CENTROID = 8,
	VS_SIG_FLUX = 9,
	VS_SIG_CUSTOM_HZ = 10,
	VS_SIG_SPECTRUM_BAND_0 = 100 /* 100..131 address spectrum[0..31] */
};

struct vs_audio_frame {
	float level;                        /* [0.0, 1.0] RMS envelope */
	float sub_bass;                     /* [0.0, 1.0] 20..60 Hz (from 4096-pt FFT) */
	float bass;                         /* [0.0, 1.0] 20..250 Hz (from 4096-pt FFT) */
	float mid;                          /* [0.0, 1.0] 250..4000 Hz (from 2048-pt FFT) */
	float high;                         /* [0.0, 1.0] 4000..20000 Hz (from 2048-pt FFT) */
	float kick;                         /* [0.0, 1.0] 40..130 Hz 4096-pt spectral-flux onset */
	float snare;                        /* [0.0, 1.0] 150..4000 Hz 2048-pt transient onset */
	float beat;                         /* [0.0, 1.0) phase-locked beat ramp/pulse */
	float spectral_centroid;            /* [0.0, 1.0] normalized spectral brightness */
	float spectral_flux;                /* [0.0, 1.0] wideband positive onset flux */
	float spectrum[VS_SPECTRUM_BANDS];  /* [0.0, 1.0] 32 log-spaced bands (dual-FFT merged) */
	float fft_bins[VS_FFT_TEX_BINS];    /* [0.0, 1.0] 512 log-freq bins for Row 0 of audio_fft */
	float waveform[VS_FFT_TEX_BINS];    /* [0.0, 1.0] 512 mono samples for Row 1 of audio_fft */
	float peak_raw;                     /* for obs-shaderfilter audio_peak compat */
	float magnitude_raw;                /* for obs-shaderfilter audio_magnitude compat */
};

struct vs_analyzer_config {
	uint32_t sample_rate;   /* e.g. 48000 */
	uint32_t fft_size_low;  /* default 4096 (11.72 Hz spacing at 48 kHz for <= 250 Hz) */
	uint32_t fft_size_mid;  /* default 2048 (23.44 Hz spacing, 42.7 ms window for > 250 Hz) */
	float sync_offset_ms;   /* -200.0 .. +500.0, default 0.0 */
	bool auto_gain;         /* default true */
	float gate_db;          /* default -55.0 dBFS */
	float floor_db;         /* manual mode floor, default -45.0 dBFS */
	float ceiling_db;       /* manual mode ceiling, default -6.0 dBFS */
	float attack_ms;        /* 0.0 .. 500.0, default 10.0 */
	float release_ms;       /* 0.0 .. 2000.0, default 180.0 */
	float kick_sensitivity; /* 0.0 .. 1.0, default 0.5 */
	float snare_sensitivity;/* 0.0 .. 1.0, default 0.5 */
};
```

### 3.2 Threading and shared tap pooling (`R-THR-1..4`, `R-TAP-1..6`)

- **Audio callback -> Ring buffer**:
  - `audio_output_callback_t` (for Track 1..6, default Track 1) and `obs_source_audio_capture_t` (for a named or parent source) run on OBS's audio thread.
  - They acquire a lightweight spin/mutex lock around `vs_audio_ring_push`, downmix the planar float channels to mono (sanitizing `NaN`/`Inf` to `0.0f`), write the mono samples and chunk timestamp into the pre-allocated 32,768-sample circular buffer, and return immediately. Zero heap allocations, zero logging, zero OBS calls.
- **Graphics thread (`video_tick`)**:
  - Each active tap instance stores `last_tick_frame_time` (`obs_get_video_frame_time()`). When multiple filters share the same tap configuration (e.g. default Track 1 with default analyzer settings), the first filter ticked in a video frame extracts the `sync_offset_ms`-shifted 4096-sample and 2048-sample windows from `vs_audio_ring`, runs `vs_analyzer_step(analyzer, dt, &frame)`, and uploads the shared `32x1` `audio_spectrum` and `512x2` `audio_fft` GPU textures; subsequent filters in the same video frame reuse `frame` and both textures with zero extra CPU or GPU work.

---

## 4. Phases, steps, named tests, and mutation checks

Every step below names its exact tests and mutation checks before any code is written (`R-TST-1..5`).

### Phase 0 -- Plan and operator decisions (this task)
1. Inspect `obs-playlister`, OBS 33.0.0 headers (`libobs/obs.h`, `plugins/nv-filters`), `exeldro/obs-shaderfilter` headers (`R-LIC-2`, `R-LIC-6`), `mmlTools/audio-wave` (`R-LIC-3`), and `exeldro/obs-move-transition` (`R-LIC-4`).
2. Write `REQUIREMENTS.md`, `PLAN.md`, and `README.md`. Leave `LICENSE` at commit `cf2f276` (GPL-2.0).
3. Commit, push once to `origin/main`, run post-push checks (`HEAD == origin/main`, 0 zero-byte files, 0 BOMs, no banned placeholder word), and stop for operator approval.

### Phase 1 -- OBS 33 build scaffold, CI, and CTest harness
Goal: `visual-sidechain` builds warning-free against OBS 33.0.0 on Windows x64 in `C:\visual-sidechain`, passes `ctest`, installs into the OBS 33 plugin layout (`visual-sidechain\visual-sidechain.dll` + `visual-sidechain\data\manifest.json`), and loads in portable OBS 33.0.0 without a `Legacy` chip. Version `0.1.0`.

1. **Step 1.1 -- Copy and adapt build system from `obs-playlister`** (read-only copy; nothing in `obs-playlister` is touched):
   - `CMakeLists.txt`, `CMakePresets.json` (`Visual Studio 17 2022`, `x64,version=10.0.26100.0`), `buildspec.json` (`obs-studio` `33.0.0`, `prebuilt` and `qt6` `2026-10-23`), `.clang-format`, `.gersemirc`, `.gitignore` allowlist, `PLUGIN-TEMPLATE.md`.
   - `cmake/common/bootstrap.cmake`, `cmake/common/buildspec_common.cmake` (with the list-argument fix and `-DENABLE_PLUGINS:BOOL=OFF`, `-DENABLE_FRONTEND:BOOL=OFF`, `-DENABLE_SCRIPTING:BOOL=OFF`), `cmake/common/manifest.json.in`, `cmake/windows/defaults.cmake`, `cmake/windows/helpers.cmake` (OBS 33 flat layout), `cmake/windows/resources/resource.rc.in`.
   - `.github/workflows/push.yaml`, `.github/workflows/build-project.yaml` (`windows-2022` runner, macOS/Ubuntu disabled), `.github/scripts/Build-Windows.ps1` (runs `ctest --output-on-failure --no-tests=error`), `.github/scripts/Package-Windows.ps1` (OBS 33 system and portable zip layouts), `installer-Windows.iss.in`.
   - **Tests**: `cmake --preset windows-x64` configures cleanly using the shared `.deps` cache (`R-PLAT-6`, `R-PLAT-7`).
2. **Step 1.2 -- Minimal module entry (`src/plugin-main.c`) and locale (`data/locale/en-US.ini`)**:
   - Exports `obs_module_name()` (`"Visual Sidechain"`), `obs_module_description()`, `obs_module_load()`, `obs_module_unload()`, logging `[visual-sidechain] plugin loaded successfully (version 0.1.0)`.
   - **Tests**: `cmake --build --preset windows-x64 --config RelWithDebInfo` compiles with 0 warnings (`CMAKE_COMPILE_WARNING_AS_ERROR` ON); `dumpbin /exports` verifies `obs_module_load`, `obs_module_unload`, `obs_module_name`, `obs_module_description`.
3. **Step 1.3 -- CTest scaffold (`tests/CMakeLists.txt`, `tests/check-locale-keys.cmake`) and portable OBS 33 smoke test**:
   - Configures test `PATH` from `OBS::libobs` and `.deps` prefix directories without copying DLLs; registers `check-locale-keys` and its negative test `check-locale-keys-fails-on-missing-key`.
   - **Tests**:
     - `ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error` passes both `check-locale-keys` and `check-locale-keys-fails-on-missing-key`.
     - Portable OBS 33.0.0 smoke test (`C:\Users\jeffr\OBS-Studio-33-Portable`): install `release/RelWithDebInfo/visual-sidechain` into `C:\Users\jeffr\OBS-Studio-33-Portable\plugins\visual-sidechain`, launch from `bin\64bit`, close with `CloseMainWindow()`, verify `[visual-sidechain] plugin loaded successfully (version 0.1.0)` and `Number of memory leaks: 0` in the OBS log, and no `Legacy` chip in Plugin Manager (`TESTING.md` 1.1).
   - **Mutation check**: Reference an undefined locale key `obs_module_text("MissingKeyProbe")` in `src/plugin-main.c`, run `ctest` to confirm `check-locale-keys` fails non-zero, then remove the probe.

### Phase 2 -- Layer 2 (Pure-C Dual-FFT Analysis + WAV Fixtures) and Layer 1 (Audio Tap)
Goal: Complete, unit-tested C17 dual-resolution FFT audio analysis engine (`src/core/`), WAV fixture test suite (`tests/fixtures/*.wav`), and OBS 33 audio tap manager (`src/obs/audio-tap.c`) defaulting to Track 1. Version `0.2.0`.

1. **Step 2.1 -- `src/core/audio-ring.c` + `tests/test-audio-ring.c`**:
   - Circular mono float buffer (capacity 32,768 samples, ~682 ms at 48 kHz), stereo/multichannel downmix helper, `NaN`/`Inf` sanitization (`R-TAP-5`), and `vs_audio_ring_read_window(ring, N, offset_samples, out_window)` supporting both 4096-sample and 2048-sample windows and positive delay offsets up to `+500` ms (`24,000` samples at 48 kHz) (`R-ANA-2`, `R-ANA-3`).
   - **Tests (`test-audio-ring`)**:
     - `test_ring_wrap_around`: exact sample readback across multiple 32,768-sample wrap-around boundaries for both `N = 4096` and `N = 2048` windows.
     - `test_ring_sync_offset_delay`: pushing a unit impulse at sample `t = 25000` and reading with `offset_samples = 0`, `4800` (`+100 ms`), and `24000` (`+500 ms`) places the impulse at the exact expected sample index in the output window.
     - `test_ring_nan_inf_sanitization`: `NaN`, `+Inf`, and `-Inf` inputs in stereo/planar channels become `0.0f` after downmix.
     - `test_ring_underrun_zero_pad`: reading before 4096 samples have been pushed pads leading zeros deterministically.
   - **Mutation check**: Off-by-one the ring wrap mask (`capacity - 2` instead of `capacity - 1` or reverse `offset_samples` sign), verify `test-audio-ring` fails non-zero, then revert.
2. **Step 2.2 -- `src/core/fft.c` + `tests/test-fft.c`**:
   - Precomputed Hann window and radix-2 Cooley-Tukey in-place complex FFT supporting **`N = 4096` (low-band 11.72 Hz spacing)** and **`N = 2048` (mid/high 23.44 Hz spacing)** (`R-ANA-4`).
   - **Tests (`test-fft`)**:
     - `test_fft_4096_subbass_resolution`: at 48 kHz, feeds two simultaneous or sequential pure sines at `35.15625 Hz` (bin 3 of `N = 4096`) and `82.03125 Hz` (bin 7 of `N = 4096`), asserting distinct peaks separated by a $> 25\text{ dB}$ valley between bins 4–5 (proving 4096-pt resolves sub-bass from upper bass).
     - `test_fft_2048_mid_high`: pure sine at `996.09375 Hz` (bin 42.5 / bin 42 of `N = 2048`) and `7992.1875 Hz` peaks at the exact expected bin with $> 40\text{ dB}$ Hann sidelobe suppression.
     - `test_fft_parseval_and_silence`: Parseval energy conservation within `1e-3`; DC peaks at bin 0; all-zero input yields all-zero magnitudes.
   - **Mutation check**: Disable the Hann window multiplier in `vs_fft_compute` (rectangular window leakage), verify `test_fft_2048_mid_high` sidelobe suppression assertion fails non-zero, then revert.
3. **Step 2.3 -- `src/core/spectrum.c` + `tests/test-spectrum.c`**:
   - Merges the **4096-point FFT** ($\le 250\text{ Hz}$) and **2048-point FFT** ($> 250\text{ Hz}$) into: (1) 32 log-spaced bands ($20\text{ Hz}..20\text{ kHz}$) with fractional bin interpolation; (2) 512 log-frequency FFT bins (`fft_bins[512]`); (3) custom Hz band extractor `vs_spectrum_extract_hz_band(freq_low_hz, freq_high_hz, mode)`; and (4) `level`, `sub_bass` ($20..60\text{ Hz}$), `bass` ($20..250\text{ Hz}$), `mid` ($250..4000\text{ Hz}$), `high` ($4000..20000\text{ Hz}$), `spectral_centroid`, and `spectral_flux` (`R-ANA-5`).
   - **Tests (`test-spectrum`)**:
     - `test_spectrum_subbass_vs_bass_4096`: a `38 Hz` sine excites `sub_bass` (`> 0.7`) and bands `1..2` while `80..250 Hz` bands (`5..10`) stay `< 0.05`; a `110 Hz` sine excites `bass` and bands `6..8` while `sub_bass` stays `< 0.05`.
     - `test_spectrum_mid_high_isolation`: `1 kHz` sine excites `mid` and bands `14..18` while `bass` and `high` stay `< 0.02`; `8 kHz` sine excites `high` and bands `25..29`.
     - `test_spectrum_custom_hz_window`: `vs_spectrum_extract_hz_band(420.0f, 460.0f, ...)` responds strongly (`> 0.8`) to a `440 Hz` sine and rejects (`< 0.05`) `220 Hz` and `880 Hz` sines.
   - **Mutation check**: Route the low bands (`<= 250 Hz`) through a 1024-pt/2048-pt uninterpolated lookup instead of the 4096-pt FFT, verify `test_spectrum_subbass_vs_bass_4096` fails non-zero, then revert.
4. **Step 2.4 -- `src/core/envelope.c` + `tests/test-envelope.c`**:
   - Per-band auto-gain with noise-gate floor (`gate_db = -55` dBFS) and slow running crest decay, manual `[floor_db, ceiling_db]` dB normalization, frame-rate-independent asymmetric `attack_ms` / `release_ms` smoothing, and 50 ms audio-starvation decay to zero (`R-ANA-6`, `R-ANA-7`).
   - **Tests (`test-envelope`)**:
     - `test_envelope_noise_gate_silence`: silence (`0.0f`) and `-70` dBFS background noise stay `0.0f` under auto-gain (never amplifies noise floor).
     - `test_envelope_autogain_40db`: a `-36` dBFS signal and a `-6` dBFS signal both converge to `[0.75, 1.0]` normalized peaks under auto-gain.
     - `test_envelope_attack_release_constants`: step response reaches $1 - e^{-1} \approx 0.632$ after `dt == attack_ms` and decays to $e^{-1} \approx 0.368$ after `dt == release_ms`.
     - `test_envelope_starvation_decay`: stopping sample input for `> 50` ms decays output smoothly to `< 1e-3`.
   - **Mutation check**: Bypass the `gate_db` floor check in auto-gain so `-70` dBFS noise is normalized to `1.0f`, verify `test_envelope_noise_gate_silence` fails non-zero, then revert.
5. **Step 2.5 -- `src/core/onset.c`, `src/core/analyzer.c`, `tests/wav-reader.h`, `tests/fixtures/*.wav` + `tests/test-onset.c`, `tests/test-analyzer.c`**:
   - 4096-pt sub-bass spectral flux detector (`40..130 Hz`) for `kick`, 2048-pt mid/high transient flux detector (`150..4000 Hz`) for `snare`, refractory cooldown, `60..180` BPM tempo/phase tracker for `beat` (including negative `sync_offset_ms` phase advance), full `vs_analyzer_step` pipeline, and deterministic WAV fixture generator/reader (`R-ANA-8..10`, `R-TST-2`).
   - **Tests (`test-onset`, `test-analyzer`)**:
     - `test_wav_kick_120bpm`: loads `tests/fixtures/kick-120bpm-4bars.wav` (48 kHz WAV with 8 kicks at `t = 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5` s and 4 snares on beats 2 and 4 at `t = 0.5, 1.5, 2.5, 3.5` s), steps the analyzer at 60 fps (`dt = 16.67 ms`), and asserts: (a) `kick` peaks `> 0.7` within $\le 35\text{ ms}$ of each of the 8 known kick timestamps and drops `< 0.2` between beats; (b) `snare` peaks `> 0.7` only on beats 2 and 4 (`t = 0.5, 1.5, 2.5, 3.5` s) and stays `< 0.15` on kick-only beats 1 and 3; (c) `beat` locks to `120` BPM ($\pm 2$ BPM).
     - `test_wav_subbass_35hz_vs_kick_95hz`: loads `tests/fixtures/subbass-35hz-vs-kick-95hz.wav` (continuous `35 Hz` sub-bass drone with `95 Hz` kick transients every `0.5` s) and asserts the 4096-pt low-band FFT detects every `95 Hz` kick onset (`kick > 0.65`) on top of the steady `35 Hz` sub-bass (`sub_bass > 0.7`).
     - `test_wav_dynamic_range_and_silence`: loads `tests/fixtures/dynamic-range-40db.wav` and `tests/fixtures/silence-and-dc.wav`, asserting auto-gain normalization and clean decay to `0.0` on silence/DC.
   - **Mutation check**: Shift the expected kick timestamps in `test_wav_kick_120bpm` by `+100 ms` (or widen refractory cooldown to `700 ms` so every second kick is missed), verify `test-analyzer` fails non-zero, then revert.
6. **Step 2.6 -- `src/obs/audio-tap.c`**:
   - Implements Track 1..6 (`obs_add_raw_audio_callback`, **defaulting to Track 1 / `mix_idx = 0`**), named source (`obs_source_add_audio_capture_callback`), and parent source (`obs_filter_get_parent` + `obs_source_add_audio_capture_callback`) taps (`R-TAP-1..6`), shared tap pooling, once-per-video-frame analyzer execution, and shared `32x1` `audio_spectrum` + `512x2` `audio_fft` GPU texture uploads (`R-THR-1..4`, `R-SHD-2`).
   - **Tests**: `ctest` regression pass + portable OBS 33.0.0 smoke test (`TESTING.md` 2.1): verify Track 1 attaches by default on filter creation, logs attach/detach cleanly, and switching between Track 1, Named Source, and Parent Source 20 times produces zero crashes and `Number of memory leaks: 0` on exit.

### Phase 3 -- Layer 3b (Audio Modulator, Baseline Save Protection, NVIDIA RTX Benchmark, and Sync Offset Lag Measurement)
Goal: The **Visual Sidechain: Audio Modulator** filter drives any numeric setting on any sibling filter (including OBS 33's NVIDIA Blur, Background Blur, and Background Removal), guarantees original baseline values are restored on disable/remove/exit/scene-save, and measures/verifies `sync_offset_ms` against real audio-to-picture lag. Version `0.3.0`.

1. **Step 3.1 -- `src/core/modulation.c`, `src/core/baseline.c` + `tests/test-modulation.c`, `tests/test-modulator-baseline.c`**:
   - `src/core/modulation.c`: bipolar `amount` (`-1.0..+1.0`), `[min_value, max_value]` mapping, integer rounding + step quantization, update-rate divider (`1/2/4` frames), and epsilon change-only gate (`R-MOD-3`, `R-MOD-4`).
   - `src/core/baseline.c`: per-`(target_filter, target_setting)` baseline table (`R-MOD-5`, `R-MOD-6`): captures original unmodulated value once before first write, detects external user edits when `current != last_written`, and produces restoration actions on slot disable, target/setting change, filter disable, filter remove/destroy, **scene-collection `save`**, and crash-recovery reload.
   - **Tests (`test-modulation`, `test-modulator-baseline`)**:
     - `test_modulation_math_and_epsilon`: positive amount `0.0 -> min`, `1.0 -> max`; negative amount (ducking) `0.0 -> max`, `1.0 -> min`; integer step rounding; rate divider (`div = 2, 4`); epsilon gate returns `changed = false` on steady input.
     - `test_baseline_capture_once_during_pulse`: target `intensity` starts at `0.25`; 120 ticks of modulation pulse it up to `0.85`; asserts captured baseline remains `0.25` (never overwritten by `0.85`).
     - `test_baseline_restore_on_disable_retarget_remove`: disabling the slot, changing `target_setting` from `intensity` to `threshold`, disabling the modulator filter, or destroying the modulator restores `intensity` to `0.25`.
     - `test_baseline_scene_save_mid_pulse`: while `intensity` is mid-pulse at `0.80`, invoking `vs_baseline_on_pre_save` restores target `intensity` to `0.25` for serialization, and the next `video_tick` resumes modulation seamlessly (`R-MOD-6`).
     - `test_baseline_crash_recovery_reload`: simulates loading a scene collection where a prior crash left `intensity = 0.80` on the target filter while the modulator's saved baseline map recorded `0.25`; asserts `vs_baseline_on_load` immediately restores `0.25`.
   - **Mutation check**: Comment out the `vs_baseline_on_pre_save` restoration in `src/core/baseline.c` (leaving the target at `0.80` during save), verify `test_baseline_scene_save_mid_pulse` fails non-zero, then revert.
2. **Step 3.2 -- `src/obs/modulator-filter.c`**:
   - Registers `visual_sidechain_modulator_filter` (`Visual Sidechain: Audio Modulator`).
   - Properties UI: Audio Tap group (**default: Track 1**, sync offset `-200..+500` ms, auto-gain, attack, release, kick/snare sensitivity) + up to 8 Modulation Slots (Target Filter dropdown via `obs_source_enum_filters`, Target Setting dropdown via recursive `obs_source_properties` inspection, Signal dropdown including Custom Hz Window, Amount slider `-100%..+100%`, Min/Max range controls seeded from target property limits, Update Rate divider `Every frame` / `Every 2 frames` / `Every 4 frames`) (`R-MOD-1..6`).
   - Hooks `video_tick`, `enable` signal, `filter_remove`, `destroy`, and `save` (`obs_source_info.save` + frontend exit/save callback) to `vs_baseline_*` (`R-MOD-5`, `R-MOD-6`).
   - **Tests**: `ctest` (including `check-locale-keys` on all new UI strings) + portable OBS 33 test (`TESTING.md` 3.1): modulate `Color Correction` `saturation` to `0.90` mid-beat, save scene collection (`obs_frontend_save`), disable/remove the modulator, and inspect the scene collection JSON on disk to verify `saturation` was saved at its original baseline value, not `0.90`.
3. **Step 3.3 -- NVIDIA RTX filter modulation benchmark (`TESTING.md` 3.2, `R-NV-1/2`)**:
   - In portable OBS 33.0.0 on the RTX 5080, drive:
     1. `nv_greenscreen_filter` (**NVIDIA Background Removal**): `threshold` (`0.0..1.0`), `processing_interval` (`1..4`), and `mode` (`0..3`).
     2. `nv_blur_filter` (**NVIDIA Blur Filter**): `intensity` (`0.0..1.0`).
     3. `nv_background_blur_filter` (**NVIDIA Background Blur Filter**): `intensity` (`0.0..1.0`).
   - **Tests**: Measure and record GPU/CPU frame render times (ms) in OBS Stats at `Every frame`, `Every 2 frames`, and `Every 4 frames`, and verify baseline restoration when disabled or saved mid-pulse.
4. **Step 3.4 -- Audio-to-picture lag measurement and `sync_offset_ms` slider verification (`TESTING.md` 3.3, `R-ANA-3`)**:
   - Generate a deterministic 60 fps test video clip (`tests/fixtures/av-sync-flash-click.mp4` or scripted source) containing a 1-frame white flash paired with a simultaneous 60 Hz + 1 kHz audio click every `1.000` s.
   - Play the clip in portable OBS 33.0.0 under two filter chains:
     1. **Bare video source** + Audio Modulator (tapping Track 1).
     2. **Video source with NVIDIA Background Removal + NVIDIA Blur** + Audio Modulator.
   - **Tests**:
     - Measure the uncompensated audio-to-picture lag in milliseconds (frame delta between the source's white flash frame and the unshifted `sync_offset_ms = 0` modulation peak).
     - Dial `sync_offset_ms` to the measured lag and verify that the modulated visual pulse lands on the exact same video frame ($\pm 1$ frame / `16.7 ms`) as the white flash, and confirm that the `-200 ms .. +500 ms` slider range covers both chains with margin. Record the measured lag values in `TESTING.md`.

### Phase 4 -- Layer 3a (Shader Filter + 9 Original FFT-Reactive Shaders)
Goal: Ship the **Visual Sidechain: Shader Filter** binding all audio uniforms + both GPU FFT textures (`32x1` `audio_spectrum` and `512x2` `audio_fft`), and all **9 original `GPL-2.0-or-later` FFT-reactive shaders** (`R-SHD-1..4`, zero Shadertoy copies). Version `0.4.0`.

1. **Step 4.1 -- Shader host implementation + headless parser tests (`src/obs/shader-filter.c`, `src/core/shader-parse.c`, `tests/test-shader-parser.c`)**:
   - Extract `#include` expansion (with a 16-level recursion depth cap and NULL quote guards, fixing `R-UP-3`) and uniform annotation parsing into `src/core/shader-parse.c` so they are headlessly tested by `test-shader-parser`.
   - Integrate the shader filter (`D-2a` fork of `exeldro/obs-shaderfilter` renamed to `visual_sidechain_shader_filter`, stripping `data/examples/` per `R-LIC-6` and replacing the `os_dlopen` `gs_image_file` hack per `R-UP-2`, or `D-2b` lean host), binding `audio_level`, `audio_sub_bass`, `audio_bass`, `audio_mid`, `audio_high`, `audio_kick`, `audio_snare`, `audio_beat`, `audio_spectral_centroid`, `audio_spectral_flux`, `audio_custom_band` (`R-SHD-1`), plus `audio_spectrum` (`32x1`) and `audio_fft` (`512x2`) (`R-SHD-2`) on every frame.
   - **Tests (`test-shader-parser`)**:
     - `test_shader_include_cycle_guard`: cyclic `#include "a.shader"` -> `#include "b.shader"` -> `#include "a.shader"` terminates cleanly at depth 16 with an error code instead of overflowing the stack (`R-UP-3`).
     - `test_shader_malformed_include_quotes`: `#include unquoted` returns an error without crashing on `NULL` `strchr`.
     - `test_shader_uniform_annotations`: parses `uniform float amount < string label = "Amount"; string widget_type = "slider"; float minimum = 0.0; float maximum = 2.0; float step = 0.01; > = 1.0;` into exact struct fields.
   - **Mutation check**: Remove the recursion depth counter check in `src/core/shader-parse.c`, verify `test_shader_include_cycle_guard` fails/aborts non-zero, then revert.
2. **Step 4.2 -- Author the 9 original `GPL-2.0-or-later` FFT-reactive shaders (`data/shaders/`, `R-SHD-3`, zero Shadertoy copies)**:
   - Write from scratch under `GPL-2.0-or-later`:
     1. `fft-spectral-aura.shader` — **FFT Spectral Edge / Border Aura** (alpha cutout silhouette & frame border glow where perimeter angle/position samples `audio_fft` / `audio_spectrum`, designed to pair with NVIDIA Background Removal).
     2. `fft-wave-warp-glitch.shader` — **FFT Displacement / Wave Warp / Glitch** (low FFT bins drive large sinusoidal warps, mid bins drive fine ripples, high-frequency spectral flux triggers horizontal scanline glitch slices).
     3. `fft-chromatic-prism-bloom.shader` — **FFT Chromatic Prism & Bloom** (bass, mid, and high FFT bins independently drive radial RGB dispersion, hue shift, and highlight bloom).
     4. `fft-visualizer-overlay.shader` — **FFT Visualizer Overlays** (32/512-bin spectrum bars with peak-hold, polar/circular spectrum ring, and oscilloscope waveform composited over video).
     5. `fft-zoom-pulse.shader` — **FFT Zoom Pulse** (radial zoom punch, barrel lens warp, and edge vignette driven by `audio_kick` / `audio_bass`).
     6. `fft-kaleidoscope.shader` — **FFT Kaleidoscope** (N-fold polar mirror symmetry where rotation, zoom, and per-sector radial offset sample `audio_fft`).
     7. `fft-feedback-trails.shader` — **FFT Feedback Trails** (uses `previous_output` ping-pong render target so FFT energy drives motion-echo persistence/decay, zoom drift, and spectral hue shift).
     8. `fft-strobe-flash.shader` — **FFT Strobe & Flash** (onset/flux-gated brightness, tint, or invert flash with a configurable max flash rate Hz cap).
     9. `fft-mirror-split.shader` — **FFT Mirror & Split** (2-way/4-way/diagonal/slice mirror where FFT bins displace slices or flip mirror axes on beats).
   - **Tests (`test-shader-parser`)**:
     - `test_builtin_shaders_parse_and_provenance`: opens all 9 `.shader` files in `data/shaders/`, verifies each has a `GPL-2.0-or-later` header, balanced braces, valid uniform annotations, references `audio_*` uniforms / `audio_fft` / `audio_spectrum`, zero occurrences of banned words or `CC BY-NC-SA`, and zero BOMs.
   - **Mutation check**: Introduce a syntax/annotation error into a temporary copy of a shader fixture, verify `test-shader-parser` rejects it non-zero, then remove.
3. **Step 4.3 -- Portable OBS 33 D3D11 compilation & stacked GPU cost verification (`TESTING.md` 4.1..4.3)**:
   - Load each of the 9 built-in shaders in portable OBS 33.0.0 on a 1080p60 source (including `fft-spectral-aura.shader` stacked directly below **NVIDIA Background Removal**), verify zero HLSL compiler warnings/errors in the OBS log, verify live FFT reactivity, and measure stacked GPU frame time (3 FFT shaders + 1 Audio Modulator driving NVIDIA Background Removal on the RTX 5080).

### Phase 5 -- Vendor API Signal Publishing & In-Process Fader Bridge (`R-VND-1..3`, `R-MOD-7`)
Goal: Expose Visual Sidechain audio/beat signals over the `obs-websocket` 5 vendor API (`vendor_name = "visual-sidechain"`, `get_signals` + 30 Hz `signals_changed`) and the in-process bridge (`visual_sidechain_get_signals` proc on `obs_get_proc_handler()` and `visual_sidechain_signals` signal on `obs_get_signal_handler()`), and consume Midi Fadester's 10 fader slots (`crossfader`, `fader_1`..`fader_8`, `master`) in-process via `midi_fadester_fader_changed` and at most one `midi_fadester_get_slot_value` poll per tick (showing `"Midi Fadester is not loaded"` next to every fader source in the Audio Modulator UI and reading `0` when the proc is missing). Version `0.5.0`.

1. **Step 5.1 -- In-Process Fader & Signal Bridge (`src/core/fader-bridge.h`, `src/core/fader-bridge.c`, `tests/test-fader-bridge.c`)**:
   - Implements the 10 canonical Fadester slots (`crossfader`, `fader_1`..`fader_8`, `master`), string/index resolution (`vs_fader_slot_from_name`), value clamping `[0.0, 1.0]`, `visual_sidechain_get_signals` proc + `visual_sidechain_signals` 30 Hz signal registration on `proc_handler_t` / `signal_handler_t`, `midi_fadester_fader_changed` listener, and at-most-one-poll-per-tick `midi_fadester_get_slot_value` consumer.
   - Extends `enum vs_signal_id` (`VS_SIG_FADER_CROSSFADER = 200` .. `VS_SIG_FADER_MASTER = 209`) and `vs_analyzer_get_signal` so any Audio Modulator slot can select `"Fader: Crossfader"`, `"Fader: Fader 1"` .. `"Fader: Fader 8"`, or `"Fader: Master"` as its driving source, and adds `fader_crossfader`, `fader_1`..`fader_8`, `fader_master` uniforms to the Shader Filter.
   - **Tests (`test-fader-bridge`)**:
     - `test_fader_slot_lookup_and_clamping`: resolves all 10 slot IDs (`crossfader`, `fader_1`..`fader_8`, `master`), rejects invalid names (`fader_9`, `fader_0`), clamps values to `[0.0, 1.0]`, and verifies `vs_analyzer_get_signal` drives `vs_mod_slot_eval`.
     - `test_signals_event_coalescing_30hz`: verifies `vs_fader_bridge_should_emit_signals` coalesces 60 fps (`dt = 16.67 ms`) updates to at most 30 events/sec (`>= 33.33 ms` between emissions) and suppresses duplicate emissions when signals are unchanged.
     - `test_in_process_bridge_missing_midi_fadester`: attaches a `proc_handler_t` without `midi_fadester_get_slot_value` and verifies `!vs_fader_bridge_is_midi_fadester_loaded()` and all 10 fader slots read `0.0f`.
     - `test_in_process_bridge_fake_proc_and_signal`: registers a fake `midi_fadester_get_slot_value` proc and `midi_fadester_fader_changed` signal on `proc_handler_t` / `signal_handler_t`, verifies at most 1 poll per tick (`poll_calls`), verifies `midi_fadester_fader_changed` updates fader slots immediately, and verifies `visual_sidechain_get_signals` proc and `visual_sidechain_signals` signal emit the exact `API-CONTRACT.md` fields (`level, bass, mid, high, kick, snare, beat_phase, bpm, ok`).
   - **Mutation check**: Break the `midi_fadester_fader_changed` slot update (or bypass the missing-proc zeroing / 30 Hz coalescer) in `src/core/fader-bridge.c`, verify `test-fader-bridge` fails non-zero, then revert.
2. **Step 5.2 -- `obs-websocket` Vendor API & Modulator UI Integration (`src/obs/vendor-api.h`, `src/obs/vendor-api.c`, `src/obs/modulator-filter.c`)**:
   - Registers `obs-websocket` vendor `"visual-sidechain"` in `obs_module_post_load()` with `get_signals` request and coalesced `signals_changed` vendor events (`R-VND-1..3`, no `get_faders` or `set_fader_value` on `"visual-sidechain"` since Midi Fadester owns faders).
   - Shows `"Midi Fadester is not loaded"` next to every fader source in the Audio Modulator UI when `midi_fadester_get_slot_value` is missing (`R-MOD-7`).
   - **Tests**: Full `ctest` suite (`12/12` including `test-fader-bridge` and `check-locale-keys`) + portable OBS 33.0.0 verification (`TESTING.md` 5.2): verify `[visual-sidechain] Registered obs-websocket vendor 'visual-sidechain' (get_signals=1)` in the OBS 33 log and `Number of memory leaks: 0` on shutdown.

---

## 5. Test strategy summary (`R-TST-1..6`)

- **Headless CTest suite (`tests/`)**:
  - Runs automatically after every code change and in CI (`ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error`).
  - Covers `test-audio-ring`, `test-fft` (dual 4096 + 2048), `test-spectrum` (including custom Hz windows), `test-envelope`, `test-onset`, `test-analyzer` (on recorded WAV fixtures in `tests/fixtures/*.wav` with known kicks and sub-bass), `test-modulation`, `test-modulator-baseline` (`R-MOD-5/6`), `test-shader-parser`, and `check-locale-keys` (including its negative test).
  - Every test executable undergoes a deliberate local **mutation check** (`R-TST-3`) before commit.
- **Portable OBS 33.0.0 integration tests (`C:\Users\jeffr\OBS-Studio-33-Portable`, `TESTING.md`)**:
  - Verifies OBS 33 non-Legacy plugin loading, Track 1 default audio tap, Audio Modulator driving standard and NVIDIA RTX (`nv-filters`) settings, baseline restoration on disable/remove/scene-save (`R-MOD-5/6`), audio-to-picture lag measurement and `sync_offset_ms` coverage (`Step 3.4`), D3D11 compilation of all 9 FFT shaders, and clean shutdown via `CloseMainWindow()` with `Number of memory leaks: 0`.

---

## 6. Risks and mitigations

| # | Risk | Mitigation |
|---|------|------------|
| R1 | **Target filter `update` cost when modulated every frame**: Driving another filter's settings calls `obs_source_update`, which invokes the target filter's `update` callback on the graphics thread. In `plugins/nv-filters/nvidia-videofx-filter.c:136-150`, changing `intensity` on `nv_blur_filter` or `nv_background_blur_filter` calls `NvVFX_Load(filter->handle_blur)`, and changing `mode` on `nv_greenscreen_filter` calls `NvVFX_Load(filter->handle)`, whereas changing `threshold` or `processing_interval` on `nv_greenscreen_filter` is a simple struct assignment. | (1) Batch all slots targeting the same filter into at most one `obs_source_update` per frame (`R-MOD-4`); (2) skip `obs_source_update` whenever the new value is within epsilon (`1e-4` float, `0` int) of the last written value; (3) provide a per-slot update-rate divider (`Every frame`, `Every 2 frames`, `Every 4 frames`); (4) benchmark each NVIDIA setting in Phase 3 (`R-NV-2`) and document which settings are per-frame safe vs rate-limited. |
| R2 | **Audio Modulator baking mid-pulse values into saved scene collections**: Because `obs_source_update` mutates the target filter's live `obs_data_t *`, saving a scene collection or exiting OBS mid-beat could save `intensity = 0.80` permanently. | `src/core/baseline.c` (`R-MOD-5`, `R-MOD-6`, tested by `test-modulator-baseline`) captures each target's original baseline once before first write, restores it on slot disable, retarget, filter disable/remove, and inside the `save` / exit callbacks before scene serialization, and persists the baseline map in the modulator's own settings for crash recovery. |
| R3 | **Audio and video sync**: OBS audio arrives in ~21.3 ms buffers (1024 frames at 48 kHz), the 4096-pt low-band FFT spans 85.3 ms (centered ~42.7 ms back), and camera capture or heavy GPU filters (such as NVIDIA Background Removal) can delay the picture by 1–5 frames relative to the sound. | Provide a `-200` ms to `+500` ms `sync_offset_ms` slider (`R-ANA-3`) backed by a 682 ms (32,768-sample) circular buffer (`R-ANA-2`), and measure actual audio-to-picture lag in Phase 3 Step 3.4 to verify the slider covers it. |
| R4 | **GPU and CPU cost of stacked effects**: Running separate FFTs, uploading separate spectrum textures, or executing redundant render passes across multiple stacked filters on a 1080p60 / 4K60 canvas can overload CPU or GPU frame budgets. | (1) Share audio taps and analyzers across filters using the same tap + settings so the dual `4096`+`2048` FFT and `32x1` / `512x2` texture uploads happen once per video frame (`R-ANA-10`, section 3.2); (2) Audio Modulator calls `obs_source_skip_video_filter` (`R-MOD-1`), costing 0 render passes; (3) measure stacked GPU frame times in OBS 33 Stats during Phase 4. |
| R5 | **OBS forum AI-tool policy & shader provenance**: The OBS Project's forum resource rules restrict plugins written with AI coding tools, and Shadertoy shaders default to GPL-incompatible `CC BY-NC-SA 3.0`. | Keep the AI coding tools disclosure honest in `README.md` (`R-ID-3`), treat **GitHub Releases** as the primary distribution channel, strip `obs-shaderfilter`'s `data/examples/` (`R-LIC-6`), and author all 9 FFT shaders from scratch under `GPL-2.0-or-later` (`R-LIC-5`, `R-SHD-3`). |
| R6 | **CI formatter strictness (`clang-format` 17 vs local 19.1.5, and `gersemi` 0.21.0)**: CI fails if `clang-format` 17 or `gersemi` 0.21.0 produces any diff or warning line. | Format all C/H files with the VS 2022 LLVM `clang-format.exe` (19.1.5) before every commit, and keep `CMakeLists.txt` / `tests/CMakeLists.txt` strictly in `obs-playlister`'s gersemi-verified 2-space, 120-col format (`R-STD-4`). |

---

## 7. Verify-at-build checklist

| # | Phase | Check | Expected |
|---|-------|-------|----------|
| V1 | 1 | Build with VS 2022 (MSVC 14.44) + SDK 26100 | `visual-sidechain.dll` compiles with 0 warnings (`CMAKE_COMPILE_WARNING_AS_ERROR` ON) |
| V2 | 1 | `dumpbin /exports visual-sidechain.dll` | `obs_module_load`, `obs_module_unload`, `obs_module_name`, `obs_module_description` exported |
| V3 | 1 | Portable OBS 33.0.0 Plugin Manager & log | Shows "Visual Sidechain 0.1.0" with no `Legacy` chip; log shows `[visual-sidechain] plugin loaded successfully (version 0.1.0)` and `Number of memory leaks: 0` on exit |
| V4 | 1 | CI on push (`windows-2022`) | Format check (`clang-format` + `gersemi`), build, `ctest --no-tests=error`, and Inno Setup `.exe` packaging all green |
| V5 | 2 | Headless `ctest` on `src/core/` + WAV fixtures | `test-audio-ring`, `test-fft` (4096+2048), `test-spectrum`, `test-envelope`, `test-onset`, `test-analyzer` (on `tests/fixtures/*.wav`), and `check-locale-keys` all pass and each passes its mutation check |
| V6 | 2 | Audio tap stress test in portable OBS 33.0.0 | Track 1 attaches by default; switching between Track 1, a named Media Source, and "This source's own audio" 20 times and deleting the source causes 0 crashes and 0 memory leaks on exit |
| V7 | 3 | Baseline restoration & scene-save check (`R-MOD-5/6`) | `test-modulator-baseline` passes; saving a scene collection in OBS 33 while a filter is pulsing writes the original baseline value to JSON, not the mid-pulse value |
| V8 | 3 | NVIDIA RTX filter modulation benchmark (`R-NV-1/2`) | Measure frame render time (ms) in OBS 33 Stats while Audio Modulator drives `nv_greenscreen_filter` (`threshold`, `processing_interval`, `mode`), `nv_blur_filter` (`intensity`), and `nv_background_blur_filter` (`intensity`); record results in `TESTING.md` |
| V9 | 3 | Audio-to-picture lag measurement (`Step 3.4`) | Measure lag (ms) on bare source and NVIDIA Background Removal + Blur source using flash+click test clip; verify `sync_offset_ms` (`-200..+500` ms) aligns visual hit within $\le 1$ frame |
| V10 | 4 | Built-in 9 FFT shaders & stacked GPU cost | All 9 original shaders in `data/shaders/` pass `test-shader-parser` and compile with 0 errors in OBS 33 D3D11; 3 stacked shaders + Audio Modulator stay within the 16.67 ms (60 fps) frame budget on the RTX 5080 |

---

## 8. Repo hygiene per change (`R-STD-1..7`)

- **Before every commit**:
  - Run `ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error` (once Phase 1 is in place) and verify the step's mutation check (`R-TST-3`).
  - Run `clang-format` on all `src/` and `tests/` `.c`/`.h` files.
  - Check for 0-byte files, UTF-8 BOMs, and banned placeholder words.
- **Push (one attempt max)**:
  - Run `git push origin main` once. If it fails, provide a copy-paste PowerShell block:
    ```powershell
    cd "C:\visual-sidechain"
    git status
    git push origin main
    ```
- **After every push**:
  - Verify `git rev-parse HEAD` matches `git rev-parse origin/main`.
  - Verify 0 zero-byte files and 0 UTF-8 BOMs across all tracked files.

---

## 9. Decisions for the operator

### D-1: Plugin identity, Track 1 default, and canonical workspace path (`SETTLED 2026-10-27`)
- **Resolution**: `name`: `visual-sidechain`, `displayName`: `Visual Sidechain` (no "OBS" in branding), `author`: `Jeffrey A. Brick`, `email`: `jbrick2070@gmail.com`, `website`: `https://github.com/jbrick2070/visual-sidechain`. Canonical local folder is **`C:\visual-sidechain`** (kept permanently here so it is never moved later). Default audio input is **Track 1 (the main mix)**. Keep the **Audio Modulator** for driving sibling filters including OBS's NVIDIA Blur, Background Blur, and Background Removal.

### D-2: Fork `exeldro/obs-shaderfilter`, or build our own shader host?
- **Comparison**:
  - **Option (a) -- Fork `exeldro/obs-shaderfilter` (v2.6.1, commit `73fb5c2`) for the Shader Filter**:
    - *Pros*: Already implements the full OBS `.shader` template wrapper (`mainImage`), `#include` expansion, automatic OBS Properties generation from shader uniform annotations (`label`, `widget_type`, `minimum`, `maximum`, `step`, `option_N`), multi-pass/feedback ping-pong textures (`previous_image`, `previous_output` — needed by `fft-feedback-trails.shader`), canvas border expansion (`expand_left/right/top/bottom`), and sRGB/HDR color-space support (`GS_CS_SRGB`, `GS_CS_SRGB_16F`, `GS_CS_709_EXTENDED`).
    - *Cleanup applied*: Strip `data/examples/` (`R-LIC-6`, contains CC BY-NC-SA 3.0 and Shadertoy files), ship our 9 original `GPL-2.0-or-later` FFT-reactive shaders (`R-SHD-3`), rename the filter ID (`visual_sidechain_shader_filter`) so it coexists with an installed `obs-shaderfilter` (`R-ID-4`), add a recursion-depth/NULL guard to `#include` parsing (`R-UP-3`), replace the `os_dlopen` magic-size `gs_image_file` workaround with direct OBS 33 calls (`R-UP-2`), and keep the file under `GPL-2.0-only` (`R-LIC-2`, which already matches our `LICENSE`).
  - **Option (b) -- Build a lean (~500-line) custom shader host from scratch**:
    - *Pros*: 100% our own `GPL-2.0-or-later` code; smaller file.
    - *Cons*: Reimplements uniform annotation parsing, ping-pong feedback textures, canvas expansion, and sRGB/HDR color-space plumbing from scratch.
- **Recommendation**: **Option (a) -- Fork `exeldro/obs-shaderfilter` for the shader filter (omitting `shader_transition` and stripping `data/examples/`)**, while keeping Layer 1 (`src/obs/audio-tap.c`), Layer 2 (`src/core/`), and Layer 3b (`src/obs/modulator-filter.c`) in clean, modular C files.

### D-3: NVIDIA's proprietary runtime beyond OBS's built-in filters (`SETTLED 2026-10-27`)
- **Resolution**: **Never.** Driving OBS 33's built-in `nv_greenscreen_filter`, `nv_blur_filter`, and `nv_background_blur_filter` through the Audio Modulator (`R-NV-1`) requires zero NVIDIA code or proprietary runtime loading in our plugin.

### D-4: How to bring in Exeldro's history if we fork (`D-2a`)
- **Recommendation**: Add `https://github.com/exeldro/obs-shaderfilter.git` as a remote (`shaderfilter`) in Phase 4 and merge `shaderfilter/master` (`--allow-unrelated-histories`), preserving his 344 commits and all nleseul / Charles Fettinger / Exeldro notices (`R-LIC-2`) while removing `data/examples/` (`R-LIC-6`) in the merge/cleanup commit.
