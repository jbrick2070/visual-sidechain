# visual-sidechain -- Plan

Status: 2026-10-27. Draft for operator approval (Phase 0: plan only, no code).
Companion to [REQUIREMENTS.md](REQUIREMENTS.md); requirement IDs (`R-...`) and
decisions (`D-...`) refer to that file.

---

## 1. Toolchain findings on this machine (checked 2026-10-27)

| Component | Found | Needed | Action |
|-----------|-------|--------|--------|
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
   - **Track 1..6 mix tap (`obs.h:932-934`, `obs.c:3153-3164`, `media-io/audio-io.c:289-315`)**: `obs_add_raw_audio_callback(size_t mix_idx, const struct audio_convert_info *conversion, audio_output_callback_t callback, void *param)` and `obs_remove_raw_audio_callback(mix_idx, callback, param)`. `mix_idx = 0` is Track 1 (the main stream/recording mix). Passing `conversion` with `format = AUDIO_FORMAT_FLOAT_PLANAR` (or `NULL` to inherit OBS's float planar mix format) delivers planar `float *` channel buffers (`data->data[ch]`, `data->frames`, `data->timestamp`) on the OBS audio output thread.
   - **Per-source audio tap (`obs.h:1347-1355`, `obs-source.c:5572-5594`)**: `obs_source_add_audio_capture_callback(obs_source_t *source, obs_source_audio_capture_t callback, void *param)` and `obs_source_remove_audio_capture_callback(...)`. Invoked from `source_output_audio_data` -> `source_signal_audio_data` (`obs-source.c:1522-1532, 1696`) after the source's audio has been resampled to OBS's float planar format and passed through the source's audio filters (`obs-source.c:4085-4100`). Passes `bool muted` (`obs-source.c:1594-1607`).
   - **Filtered source's own audio (`obs.h:1167`)**: `obs_filter_get_parent(filter)` returns the `obs_source_t *` to which the filter is attached; if `(obs_source_get_output_flags(parent) & OBS_SOURCE_AUDIO) != 0`, the same `obs_source_add_audio_capture_callback` attaches to `parent`.
2. **Driving another filter's settings in OBS 33 (`libobs/obs-source.c`)**:
   - Calling `obs_source_update(target_filter, NULL)` after mutating `obs_data_t *ss = obs_source_get_settings(target_filter)` checks `target_filter->info.output_flags & OBS_SOURCE_VIDEO` (`obs-source.c:1112-1114`): for a video filter it increments `target_filter->defer_update_count`, and `obs_source_video_tick` runs `obs_source_deferred_update` (`target_filter->info.update`) on the graphics thread (`obs-source.c:1093-1101, 1410-1411`).
   - Reference pattern in `exeldro/obs-move-transition` (`move-value-filter.c:878-948, 1826-2048` at `c1f1e3d`): enumerates sibling filters with `obs_source_enum_filters(parent, ...)`, holds the chosen filter via `obs_weak_source_t *`, walks `obs_source_properties(target)` (recursing into `OBS_PROPERTY_GROUP`) to list numeric `OBS_PROPERTY_FLOAT` and `OBS_PROPERTY_INT` settings and read their min/max/step limits, and updates `obs_source_get_settings(target)` + `obs_source_update(target, NULL)` in `video_tick`.
3. **OBS 33 built-in NVIDIA RTX video filters (`plugins/nv-filters/nvidia-videofx-filter.c` at `33.0.0`)**:
   - Three filters registered (`lines 1189-1235`):
     - `nv_greenscreen_filter` ("NVIDIA Background Removal"): settings `threshold` (`OBS_PROPERTY_FLOAT` slider `0.0..1.0`, step `0.05`, default `1.0`, line 1064), `mode` (`OBS_PROPERTY_LIST` int: `0` Quality, `1` Performance, `2` Quality+Chair, `3` Performance+Chair, lines 1058-1062), `processing_interval` ("Mask Refresh Every N Frames", `OBS_PROPERTY_INT` slider `1..4`, step `1`, default `1`, line 1065).
     - `nv_blur_filter` ("NVIDIA Blur Filter"): setting `intensity` (`OBS_PROPERTY_FLOAT` slider `0.0..1.0`, step `0.05`, default `0.5`, line 1055).
     - `nv_background_blur_filter` ("NVIDIA Background Blur Filter"): setting `intensity` (`OBS_PROPERTY_FLOAT` slider `0.0..1.0`, step `0.05`, default `0.5`, line 1055).
   - **Critical `update` cost finding (`nvidia-videofx-filter.c:125-151`)**:
     - In `nvvfx_filter_update`, updating `threshold` (line 131) or `processing_interval` (line 132) merely assigns a struct member (`filter->threshold`, `filter->processing_interval`) used by the shader in `draw_greenscreen_blur` (line 831) — **near-zero CPU/GPU overhead per frame**.
     - Updating `intensity` on `nv_blur_filter` or `nv_background_blur_filter` when `filter->strength != strength` calls `NvVFX_SetF32(filter->handle_blur, NVVFX_STRENGTH, filter->strength)` followed by **`NvVFX_Load(filter->handle_blur)`** (lines 145-149).
     - Updating `mode` on `nv_greenscreen_filter` when `filter->mode != mode` calls `NvVFX_SetU32` followed by **`NvVFX_Load(filter->handle)`** (lines 136-142).
     - This confirms the project brief's first risk: driving `obs_source_update` every frame on filters that call `NvVFX_Load` inside `update` can stall the graphics thread unless guarded by a change-only epsilon check, step quantization, and an optional frame-rate divider (`R-MOD-4`, `R-NV-2`).
4. **License header check of `exeldro/obs-shaderfilter` (`73fb5c2`, v2.6.1, 2026-10-24)**:
   - Recorded in detail in `REQUIREMENTS.md` `R-LIC-2` and `R-LIC-6`: `LICENSE` is GPL-2.0; `obs-shaderfilter.c` has no "or later" clause; 6 example shaders state `GPLv2` only; and 12+ example shaders in `data/examples/` are `CC BY-NC-SA 3.0` or Shadertoy ports. Any fork stays `GPL-2.0-only`, preserves nleseul / Charles Fettinger / Exeldro notices, and excludes `data/examples/`.

---

## 3. Target architecture (three layers)

```
src/
  plugin-main.c                  module entry; exports obs_module_name/description;
                                 initializes shared audio tap registry; registers filters
  plugin-support.c.in / .h       template logging ([visual-sidechain] prefix)
  core/                          Layer 2 + pure modulation math (C17, libobs util only,
                                 zero OBS source/graphics calls, 100% headless ctest)
    audio-ring.h / .c            circular mono float buffer + timestamp/sample sync offset cursor
    fft.h / .c                   precomputed Hann window + in-place radix-2 complex FFT (N=2048)
    spectrum.h / .c              FFT bins -> 32 log-spaced bands (20 Hz..20 kHz) + level/bass/mid/high
    envelope.h / .c              per-band auto-gain (with noise-gate floor) + attack/release smoothing
    onset.h / .c                 spectral-flux kick & snare onset detectors + 60..180 BPM beat phase
    analyzer.h / .c              combines ring + FFT + spectrum + envelope + onset into vs_audio_frame
    modulation.h / .c            bipolar slot math, range mapping, int rounding, epsilon change gate
  obs/                           Layer 1 + Layer 3 OBS integration
    audio-tap.h / .c             Layer 1: shared/ref-counted taps for Track 1..6 (obs_add_raw_audio_callback),
                                 named source, and parent source (obs_source_add_audio_capture_callback);
                                 runs vs_analyzer once per video frame per unique tap configuration
    modulator-filter.h / .c      Layer 3b: Visual Sidechain: Audio Modulator filter (up to 8 slots,
                                 sibling filter & property introspection, batched obs_source_update)
    shader-filter.h / .c         Layer 3a: Visual Sidechain: Shader Filter (forked from obs-shaderfilter.c
                                 per D-2a or lean host per D-2b; binds audio_* uniforms + 32x1 spectrum texture)
data/
  locale/en-US.ini               all UI strings (verified by check-locale-keys.cmake)
  internal/render_output.effect  pass-through output effect
  shaders/                       original GPL-2.0-or-later audio-reactive shaders (R-SHD-3)
    beat-pulse-zoom.shader
    kick-chromatic-aberration.shader
    snare-glitch-slice.shader
    bass-shake.shader
    audio-bloom-glow.shader
    beat-strobe-flash.shader
    audio-wave-warp.shader
    spectrum-bars-overlay.shader
tests/
  CMakeLists.txt                 headless CTest runner (DLL PATH from .deps, no copying)
  check-locale-keys.cmake        verifies every obs_module_text/T_ key exists in en-US.ini
  test-audio-ring.c
  test-fft.c
  test-spectrum.c
  test-envelope.c
  test-onset.c
  test-analyzer.c
  test-modulation.c
```

### 3.1 Core data contracts (`src/core/analyzer.h`, `src/core/modulation.h`)

```c
#define VS_SPECTRUM_BANDS 32

enum vs_signal_id {
	VS_SIG_LEVEL = 0,
	VS_SIG_BASS = 1,
	VS_SIG_MID = 2,
	VS_SIG_HIGH = 3,
	VS_SIG_KICK = 4,
	VS_SIG_SNARE = 5,
	VS_SIG_BEAT = 6,
	VS_SIG_SPECTRUM_BAND_0 = 100 /* 100..131 address spectrum[0..31] */
};

struct vs_audio_frame {
	float level;                       /* [0.0, 1.0] RMS envelope */
	float bass;                        /* [0.0, 1.0] 20..250 Hz */
	float mid;                         /* [0.0, 1.0] 250..4000 Hz */
	float high;                        /* [0.0, 1.0] 4000..20000 Hz */
	float kick;                        /* [0.0, 1.0] sub-bass onset pulse */
	float snare;                       /* [0.0, 1.0] mid/high transient pulse */
	float beat;                        /* [0.0, 1.0) phase-locked beat ramp/pulse */
	float spectrum[VS_SPECTRUM_BANDS]; /* [0.0, 1.0] 32 log-spaced bands */
	float peak_raw;                    /* for obs-shaderfilter audio_peak compat */
	float magnitude_raw;               /* for obs-shaderfilter audio_magnitude compat */
};

struct vs_analyzer_config {
	uint32_t sample_rate;   /* e.g. 48000 */
	uint32_t fft_size;      /* 1024, 2048 (default), or 4096 */
	float sync_offset_ms;   /* -200.0 .. +200.0, default 0.0 */
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
  - `audio_output_callback_t` (for Track 1..6) and `obs_source_audio_capture_t` (for a source) run on OBS's audio thread.
  - They acquire a lightweight spin/mutex lock around `vs_audio_ring_push`, downmix the planar float channels to mono (sanitizing `NaN`/`Inf` to `0.0f`), write the mono samples and the chunk timestamp into the pre-allocated circular buffer, and return immediately. Zero heap allocations, zero logging, zero OBS calls.
- **Graphics thread (`video_tick`)**:
  - Each active tap instance stores `last_tick_frame_time` (`obs_get_video_frame_time()`). When multiple filters share the same tap configuration (e.g. default Track 1 with default analyzer settings), the first filter ticked in a video frame extracts the `sync_offset_ms`-shifted window from `vs_audio_ring`, runs `vs_analyzer_step(analyzer, dt, &frame)`, and uploads the shared `32x1` `GS_R32F` / `GS_RGBA` spectrum texture; subsequent filters in the same video frame reuse `frame` and the texture with zero extra CPU or GPU work.

---

## 4. Phases and steps

### Phase 0 -- Plan and operator decisions (this task)
1. Inspect `obs-playlister`, OBS 33.0.0 headers (`libobs/obs.h`, `plugins/nv-filters`), `exeldro/obs-shaderfilter` headers (`R-LIC-2`, `R-LIC-6`), `mmlTools/audio-wave` (`R-LIC-3`), and `exeldro/obs-move-transition` (`R-LIC-4`).
2. Write `REQUIREMENTS.md`, `PLAN.md`, and `README.md`. Leave `LICENSE` at commit `cf2f276` (GPL-2.0).
3. Commit, push once to `origin/main`, run post-push checks (`HEAD == origin/main`, 0 zero-byte files, 0 BOMs, no banned placeholder word), and stop for operator approval and decisions `D-1..D-4`.

### Phase 1 -- OBS 33 build scaffold, CI, and CTest harness
Goal: `visual-sidechain` builds warning-free against OBS 33.0.0 on Windows x64, passes `ctest`, installs into the OBS 33 plugin layout (`visual-sidechain\visual-sidechain.dll` + `visual-sidechain\data\manifest.json`), and loads in portable OBS 33.0.0 without a `Legacy` chip. Version `0.1.0`.

1. **Copy and adapt build system from `obs-playlister`** (read-only copy; nothing in `obs-playlister` is touched):
   - `CMakeLists.txt`, `CMakePresets.json` (`Visual Studio 17 2022`, `x64,version=10.0.26100.0`), `buildspec.json` (`obs-studio` `33.0.0`, `prebuilt` and `qt6` `2026-10-23`), `.clang-format`, `.gersemirc`, `.gitignore` allowlist, `PLUGIN-TEMPLATE.md`.
   - `cmake/common/bootstrap.cmake`, `cmake/common/buildspec_common.cmake` (with the list-argument fix and `-DENABLE_PLUGINS:BOOL=OFF`, `-DENABLE_FRONTEND:BOOL=OFF`, `-DENABLE_SCRIPTING:BOOL=OFF`), `cmake/common/manifest.json.in`, `cmake/windows/defaults.cmake`, `cmake/windows/helpers.cmake` (OBS 33 flat layout), `cmake/windows/resources/resource.rc.in`.
   - `.github/workflows/push.yaml`, `.github/workflows/build-project.yaml` (`windows-2022` runner, macOS/Ubuntu disabled), `.github/scripts/Build-Windows.ps1` (runs `ctest --output-on-failure --no-tests=error`), `.github/scripts/Package-Windows.ps1` (OBS 33 system and portable zip layouts), `installer-Windows.iss.in`.
2. **Minimal module entry (`src/plugin-main.c`) and locale (`data/locale/en-US.ini`)**:
   - Exports `obs_module_name()` (`"Visual Sidechain"`), `obs_module_description()`, `obs_module_load()`, `obs_module_unload()`, logging load/unload with version.
3. **CTest scaffold (`tests/CMakeLists.txt`, `tests/check-locale-keys.cmake`)**:
   - Configures test `PATH` from `OBS::libobs` and `.deps` prefix directories without copying DLLs; runs `check-locale-keys` and its negative test `check-locale-keys-fails-on-missing-key`.
4. **Tests for Phase 1**:
   - `cmake --preset windows-x64` && `cmake --build --preset windows-x64 --config RelWithDebInfo` (0 warnings).
   - `ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error` passes.
   - Smoke test in portable OBS 33.0.0 (`C:\Users\jeffr\OBS-Studio-33-Portable`): copy `release/RelWithDebInfo/visual-sidechain` into `C:\Users\jeffr\OBS-Studio-33-Portable\plugins\visual-sidechain`, launch from `bin\64bit`, close with `CloseMainWindow()`, verify `[visual-sidechain] plugin loaded successfully (version 0.1.0)` in the OBS log and no `Legacy` flag in Plugin Manager.

### Phase 2 -- Layer 2 (Pure-C Analysis) and Layer 1 (Audio Tap)
Goal: Complete, unit-tested C17 audio analysis engine (`src/core/`) and OBS 33 audio tap manager (`src/obs/audio-tap.c`). Version `0.2.0`.

1. **Step 2.1 -- `src/core/audio-ring.c` + `tests/test-audio-ring.c`**:
   - Circular mono float buffer (capacity 16,384 samples), stereo/multichannel downmix helper, `NaN`/`Inf` sanitization (`R-TAP-5`), and `vs_audio_ring_read_window(ring, N, offset_samples, out_window)` supporting positive delay offsets (`R-ANA-2`, `R-ANA-3`).
   - **CTest (`test-audio-ring`)**: exact sample readback across wrap-around boundaries; positive `offset_samples` reads historical samples; `NaN`/`+Inf`/`-Inf` inputs become `0.0f`; underrun before buffer fills pads leading zeros deterministically.
2. **Step 2.2 -- `src/core/fft.c` + `tests/test-fft.c`**:
   - Precomputed Hann window and radix-2 Cooley-Tukey in-place complex FFT (`N = 1024, 2048, 4096`) (`R-ANA-4`).
   - **CTest (`test-fft`)**: Parseval energy conservation check; pure sine at bin-centered frequency peaks at the exact bin with > 40 dB sidelobe suppression under Hann window; DC signal peaks at bin 0; silence yields all-zero magnitudes; impulse yields flat magnitude spectrum.
3. **Step 2.3 -- `src/core/spectrum.c` + `tests/test-spectrum.c`**:
   - Maps FFT magnitudes to 32 log-spaced bands ($20\text{ Hz}..20\text{ kHz}$) with fractional bin interpolation for narrow low-frequency bands, plus `level`, `bass` ($20..250\text{ Hz}$), `mid` ($250..4000\text{ Hz}$), and `high` ($4000..20000\text{ Hz}$) (`R-ANA-5`).
   - **CTest (`test-spectrum`)**: 60 Hz sine excites `bass` and low spectrum bands (`0..5`) while `mid` and `high` stay `< 0.02`; 1 kHz sine excites `mid` and mid bands (`14..18`); 8 kHz sine excites `high` and upper bands (`25..29`); every band has non-zero support (no dead bass bands at 48 kHz with $N = 2048$).
4. **Step 2.4 -- `src/core/envelope.c` + `tests/test-envelope.c`**:
   - Per-band auto-gain with noise-gate floor (`gate_db = -55` dBFS) and slow running crest decay, manual `[floor_db, ceiling_db]` dB normalization, and frame-rate-independent asymmetric `attack_ms` / `release_ms` smoothing (`R-ANA-6`, `R-ANA-7`).
   - **CTest (`test-envelope`)**: silence (`0.0f` and `-70` dBFS noise floor) stays `0.0f` under auto-gain (never amplifies silence); a `-30` dBFS signal and a `-6` dBFS signal both converge to similar `[0.7, 1.0]` normalized peaks under auto-gain; step response reaches $1 - e^{-1} \approx 0.632$ after `dt == attack_ms` and decays to $e^{-1} \approx 0.368$ after `dt == release_ms`.
5. **Step 2.5 -- `src/core/onset.c`, `src/core/analyzer.c` + `tests/test-onset.c`, `tests/test-analyzer.c`**:
   - Sub-bass spectral flux detector for `kick`, mid/high transient flux detector for `snare`, refractory cooldown, and `60..180` BPM tempo/phase tracker for `beat` (including negative `sync_offset_ms` phase advance) (`R-ANA-8`, `R-ANA-9`, `R-ANA-10`).
   - **CTest (`test-onset`, `test-analyzer`)**: synthesized 120 BPM 48 kHz audio stream with 60 Hz exponentially decaying sine bursts on beats 1, 2, 3, 4 (kicks) and noise+250 Hz bursts on beats 2 and 4 (snares): verifies `kick` fires on all 4 beats, `snare` fires selectively on beats 2 and 4, and `beat` locks to a 0.5 s period (`120` BPM $\pm 2$ BPM) and wraps `[0.0, 1.0)` on each kick.
6. **Step 2.6 -- `src/obs/audio-tap.c`**:
   - Implements Track 1..6 (`obs_add_raw_audio_callback`), named source (`obs_source_add_audio_capture_callback`), and parent source (`obs_filter_get_parent` + `obs_source_add_audio_capture_callback`) taps (`R-TAP-1..6`), shared tap pooling, and once-per-video-frame analyzer execution (`R-THR-1..4`).
   - **Portable OBS 33 test**: attach a diagnostic test filter or modulator to a media source playing a test audio file in portable OBS 33.0.0; verify Track 1, named source, and parent source taps all log clean attach/detach lines and produce live `vs_audio_frame` values.

### Phase 3 -- Layer 3b (Audio Modulator filter + NVIDIA RTX filter verification)
Goal: The **Visual Sidechain: Audio Modulator** filter drives any numeric setting on any sibling filter on the same source, including OBS 33's built-in NVIDIA RTX filters. Version `0.3.0`.

1. **Step 3.1 -- `src/core/modulation.c` + `tests/test-modulation.c`**:
   - Pure-C slot evaluation: bipolar `amount` (`-1.0..+1.0`), `[min_value, max_value]` mapping, integer rounding + step quantization, and epsilon change-only gate (`R-MOD-3`, `R-MOD-4`).
   - **CTest (`test-modulation`)**: positive amount `0.0 -> min`, `1.0 -> max`; negative amount (ducking) `0.0 -> max`, `1.0 -> min`; `amount = 0.0` holds baseline; integer quantization and step rounding; epsilon gate returns `changed = false` when signal is steady.
2. **Step 3.2 -- `src/obs/modulator-filter.c`**:
   - Registers `visual_sidechain_modulator_filter` (`Visual Sidechain: Audio Modulator`).
   - Properties UI: Audio Tap group (Track 1..6 / Specific Source / This Source's Audio, sync offset `-200..+200` ms, auto-gain, attack, release, kick/snare sensitivity) + up to 8 Modulation Slots (Target Filter dropdown via `obs_source_enum_filters`, Target Setting dropdown via recursive `obs_source_properties` inspection, Signal dropdown, Amount slider `-100%..+100%`, Min/Max range controls seeded from target property limits, Update Rate divider `Every frame` / `Every 2 frames` / `Every 4 frames`) (`R-MOD-1..5`).
   - `video_tick`: evaluates active slots, batches updates per target filter into a single `obs_source_update(target, NULL)` call when values change beyond epsilon, and restores baseline values when disabled or destroyed.
3. **Step 3.3 -- NVIDIA RTX filter & built-in filter integration tests (`TESTING.md`)**:
   - In portable OBS 33.0.0 on the RTX 5080, test driving:
     1. Standard OBS `Color Correction` (`contrast`, `brightness`, `saturation`, `hue_shift`) and `Crop/Pad` (`left`, `top`, `right`, `bottom`).
     2. `nv_greenscreen_filter` (**NVIDIA Background Removal**): `threshold` (`0.0..1.0`), `processing_interval` (`1..4`), and `mode` (`0..3`).
     3. `nv_blur_filter` (**NVIDIA Blur Filter**): `intensity` (`0.0..1.0`).
     4. `nv_background_blur_filter` (**NVIDIA Background Blur Filter**): `intensity` (`0.0..1.0`).
   - Measure and record frame-render times in OBS Stats for each NVIDIA setting at `Every frame`, `Every 2 frames`, and `Every 4 frames` (`R-NV-2`, Risk R1).

### Phase 4 -- Layer 3a (Shader Filter + Built-In Audio-Reactive Shader Suite)
Goal: Ship the **Visual Sidechain: Shader Filter** with all seven audio uniforms + `32x1` `audio_spectrum` texture and a suite of original GPL-2.0-or-later VJ shaders. Version `0.4.0`.

1. **Step 4.1 -- Shader host implementation (per operator decisions `D-2` and `D-4`)**:
   - *If D-2a (recommended fork of `exeldro/obs-shaderfilter`)*:
     - Bring in `exeldro/obs-shaderfilter` history via remote merge (`D-4`), keeping all nleseul / Charles Fettinger / Exeldro notices (`R-LIC-2`), remove `data/examples/` (`R-LIC-6`, `R-UP-7`), rename filter ID to `visual_sidechain_shader_filter` (`Visual Sidechain: Shader Filter`, `R-ID-4`), replace the `os_dlopen` magic-size `gs_image_file` hack with OBS 33 `<graphics/image-file.h>` calls (`R-UP-2`), add recursion-depth and NULL guards to `load_shader_from_file` (`R-UP-3`), and replace `obs_volmeter_t` with `vs_audio_tap` (`R-UP-6`).
   - *If D-2b (custom lean shader host)*:
     - Implement `src/obs/shader-filter.c` supporting `.shader` (wrapped in `effect_template_begin/end`) and `.effect` files, uniform annotation discovery into OBS Properties, and explicit error display (`R-SHD-4`).
   - In either case, bind `audio_level`, `audio_bass`, `audio_mid`, `audio_high`, `audio_kick`, `audio_snare`, `audio_beat` (`R-SHD-1`) and the `32x1` dynamic texture `audio_spectrum` (`R-SHD-2`) on every frame.
2. **Step 4.2 -- Original GPL-2.0-or-later audio-reactive shader suite (`data/shaders/`, `R-SHD-3`)**:
   - Author and ship 8 clean, well-commented, GPL-2.0-or-later shaders:
     1. `beat-pulse-zoom.shader` (scale/zoom punch around configurable anchor driven by `audio_kick` / `audio_bass`).
     2. `kick-chromatic-aberration.shader` (radial RGB channel split driven by `audio_kick`).
     3. `snare-glitch-slice.shader` (horizontal scanline slice displacement + RGB shift triggered by `audio_snare`).
     4. `bass-shake.shader` (directional/rotational camera shake driven by `audio_bass`).
     5. `audio-bloom-glow.shader` (thresholded highlight bloom pulsed by `audio_level` / `audio_mid`).
     6. `beat-strobe-flash.shader` (exposure/brightness/tint flash synced to `audio_beat` / `audio_kick`).
     7. `audio-wave-warp.shader` (UV sine/ripple distortion modulated by `audio_bass` and `audio_high`).
     8. `spectrum-bars-overlay.shader` (32-band EQ bar / curve overlay sampling `audio_spectrum` over or under the source).
3. **Step 4.3 -- Integration & GPU cost verification (`TESTING.md`)**:
   - Load each of the 8 built-in shaders in portable OBS 33.0.0 on a 1080p60 source, verify zero HLSL compiler warnings/errors, verify audio reactivity and `audio_spectrum` texture updates, and measure stacked GPU frame time (e.g. 3 shader filters + 1 Audio Modulator driving NVIDIA Background Removal on the RTX 5080).

---

## 5. Test strategy summary (`R-TST-1..5`)

- **Headless CTest suite (`tests/`)**:
  - Runs automatically after every code change and in CI (`ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error`).
  - Covers `audio-ring`, `fft`, `spectrum`, `envelope`, `onset`, `analyzer`, `modulation`, and `check-locale-keys` (including its negative test).
- **Portable OBS 33.0.0 integration tests (`C:\Users\jeffr\OBS-Studio-33-Portable`, `TESTING.md`)**:
  - Verifies OBS 33 non-Legacy plugin loading, Track 1 / named source / parent source audio taps, Audio Modulator driving standard and NVIDIA RTX (`nv-filters`) settings, shader compilation and uniform/texture binding, and clean shutdown via `CloseMainWindow()` with zero leaks in OBS's log (`Number of memory leaks: 0`).

---

## 6. Risks and mitigations

| # | Risk | Mitigation |
|---|------|------------|
| R1 | **Target filter `update` cost when modulated every frame**: Driving another filter's settings calls `obs_source_update`, which invokes the target filter's `update` callback on the graphics thread. As found in `plugins/nv-filters/nvidia-videofx-filter.c:136-150`, changing `intensity` on `nv_blur_filter` or `nv_background_blur_filter` calls `NvVFX_Load(filter->handle_blur)`, and changing `mode` on `nv_greenscreen_filter` calls `NvVFX_Load(filter->handle)`, whereas changing `threshold` or `processing_interval` on `nv_greenscreen_filter` is a simple struct assignment. | (1) Batch all slots targeting the same filter into at most one `obs_source_update` per frame (`R-MOD-4`); (2) skip `obs_source_update` whenever the new value is within epsilon (`1e-4` float, `0` int) of the last written value; (3) provide a per-slot update-rate divider (`Every frame`, `Every 2 frames`, `Every 4 frames`); (4) benchmark each NVIDIA setting in Phase 3 (`R-NV-2`) and document which settings are per-frame safe vs rate-limited. |
| R2 | **Audio and video sync**: OBS audio arrives in ~21.3 ms buffers (1024 frames at 48 kHz) while video renders every 16.67 ms (60 fps), and camera capture or heavy GPU filters (such as NVIDIA Background Removal) can delay the picture by 1-3 frames relative to the sound. | Provide a `-200` ms to `+200` ms `sync_offset_ms` slider (`R-ANA-3`) backed by a 341 ms circular sample buffer (`R-ANA-2`): positive offsets delay the audio analysis cursor so visual hits land on the delayed picture; negative offsets advance the beat phase predictor (`beat`). |
| R3 | **GPU and CPU cost of stacked effects**: Running separate FFTs, uploading separate spectrum textures, or executing redundant render passes across multiple stacked filters on a 1080p60 / 4K60 canvas can overload CPU or GPU frame budgets. | (1) Share audio taps and analyzers across filters using the same tap + settings so FFT and `32x1` spectrum texture upload happen once per video frame (`R-ANA-10`, section 3.2); (2) Audio Modulator calls `obs_source_skip_video_filter` (`R-MOD-1`), costing 0 render passes; (3) measure stacked GPU frame times in OBS 33 Stats during Phase 4. |
| R4 | **OBS forum AI-tool policy**: The OBS Project's forum resource rules restrict or block plugins written with AI coding tools from being listed on the OBS forum resource directory. | Keep the AI coding tools disclosure honest and prominent in `README.md` (`R-ID-3`); treat **GitHub Releases** (shipping the `.exe` installer, system `.zip`, and portable `.zip`) as the primary distribution channel while keeping `data/manifest.json` complete. |
| R5 | **CI formatter strictness (`clang-format` 17 vs local 19.1.5, and `gersemi` 0.21.0)**: CI fails if `clang-format` 17 or `gersemi` 0.21.0 produces any diff or warning line. | Format all C/H files with the VS 2022 LLVM `clang-format.exe` (19.1.5) before every commit, and keep `CMakeLists.txt` / `tests/CMakeLists.txt` strictly in `obs-playlister`'s gersemi-verified 2-space, 120-col format (`R-STD-4`). |

---

## 7. Verify-at-build checklist

| # | Phase | Check | Expected |
|---|-------|-------|----------|
| V1 | 1 | Build with VS 2022 (MSVC 14.44) + SDK 26100 | `visual-sidechain.dll` compiles with 0 warnings (`CMAKE_COMPILE_WARNING_AS_ERROR` ON) |
| V2 | 1 | `dumpbin /exports visual-sidechain.dll` | `obs_module_load`, `obs_module_unload`, `obs_module_name`, `obs_module_description` exported |
| V3 | 1 | Portable OBS 33.0.0 Plugin Manager & log | Shows "Visual Sidechain 0.1.0" with no `Legacy` chip; log shows `[visual-sidechain] plugin loaded successfully (version 0.1.0)` and `Number of memory leaks: 0` on exit |
| V4 | 1 | CI on push (`windows-2022`) | Format check (`clang-format` + `gersemi`), build, `ctest --no-tests=error`, and Inno Setup `.exe` packaging all green |
| V5 | 2 | Headless `ctest` on `src/core/` | `test-audio-ring`, `test-fft`, `test-spectrum`, `test-envelope`, `test-onset`, `test-analyzer`, and `check-locale-keys` all pass |
| V6 | 2 | Audio tap stress test in portable OBS 33.0.0 | Switching between Track 1, a named Media Source, and "This source's own audio" 20 times and deleting the source causes 0 crashes and 0 memory leaks on exit |
| V7 | 3 | NVIDIA RTX filter modulation benchmark (`R-NV-1/2`) | Measure frame render time (ms) in OBS 33 Stats while Audio Modulator drives `nv_greenscreen_filter` (`threshold`, `processing_interval`, `mode`), `nv_blur_filter` (`intensity`), and `nv_background_blur_filter` (`intensity`); record results in `TESTING.md` |
| V8 | 4 | Built-in shader suite & stacked GPU cost | All 8 shaders in `data/shaders/` compile with 0 errors, react to `audio_*` uniforms and `audio_spectrum`, and 3 stacked shaders + Audio Modulator stay well within the 16.67 ms (60 fps) frame budget on the RTX 5080 |

---

## 8. Repo hygiene per change (`R-STD-1..7`)

- **Before every commit**:
  - Run `ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error` (once Phase 1 is in place).
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

## 9. Decisions for the operator (needed before Phase 1 code begins)

### D-1: Plugin identity and workspace path
- **Question**: Approve `name`: `visual-sidechain`, `displayName`: `Visual Sidechain` (no "OBS" in branding), `author`: `Jeffrey A. Brick`, `email`: `jbrick2070@gmail.com`, `website`: `https://github.com/jbrick2070/visual-sidechain`, and `C:\visual-sidechain` as the local repository path?
- **Recommendation**: Approve.

### D-2: Fork `exeldro/obs-shaderfilter`, or build our own shader host?
- **Comparison**:
  - **Option (a) -- Fork `exeldro/obs-shaderfilter` (v2.6.1, commit `73fb5c2`) for the Shader Filter**:
    - *Pros*: Already implements the full OBS `.shader` template wrapper (`mainImage`), `#include` expansion, Shadertoy/GLSL-to-HLSL syntax helpers, automatic OBS Properties generation from shader uniform annotations (`label`, `widget_type`, `minimum`, `maximum`, `step`, `option_N`), multi-pass/feedback ping-pong textures (`previous_image`, `previous_output`), canvas border expansion (`expand_left/right/top/bottom`), sRGB/HDR color-space support (`GS_CS_SRGB`, `GS_CS_SRGB_16F`, `GS_CS_709_EXTENDED`), and 100% compatibility with existing community `.shader` and `.effect` files.
    - *Cons / Cleanup needed*: Must strip `data/examples/` (`R-LIC-6`, contains CC BY-NC-SA 3.0 and Shadertoy files), rename the filter ID (`visual_sidechain_shader_filter`) so it coexists with an installed `obs-shaderfilter` (`R-ID-4`), add a recursion-depth/NULL guard to `#include` parsing (`R-UP-3`), replace the `os_dlopen` magic-size `gs_image_file` workaround with direct OBS 33 calls (`R-UP-2`), and keep the file under `GPL-2.0-only` (`R-LIC-2`, which already matches our `LICENSE`).
  - **Option (b) -- Build a lean (~500-line) custom shader host from scratch**:
    - *Pros*: 100% our own `GPL-2.0-or-later` code; smaller file; no legacy transition or GLSL-converter code.
    - *Cons*: Would have to reimplement uniform annotation parsing for sliders/colors, ping-pong feedback textures, canvas expansion, and sRGB/HDR color-space plumbing from scratch, or lose compatibility with existing `.shader` files that rely on those features.
- **Recommendation**: **Option (a) -- Fork `exeldro/obs-shaderfilter` for the shader filter (omitting `shader_transition` and stripping `data/examples/`)**, while keeping Layer 1 (`src/obs/audio-tap.c`), Layer 2 (`src/core/`), and Layer 3b (`src/obs/modulator-filter.c`) in clean, modular C files.

### D-3: NVIDIA's proprietary runtime beyond OBS's built-in filters
- **Question**: Should Visual Sidechain load NVIDIA's proprietary Maxine runtime directly (for effects beyond OBS 33's built-in Background Removal, Blur, and Background Blur): **yes**, **later**, or **never**?
- **Recommendation**: **Never** (or **Later** only if a specific effect is wanted). Driving OBS 33's built-in `nv_greenscreen_filter`, `nv_blur_filter`, and `nv_background_blur_filter` through the Audio Modulator (`R-NV-1`) requires zero NVIDIA code or proprietary runtime loading in our plugin.

### D-4: How to bring in Exeldro's history if we fork (`D-2a`)
- **Question**: If we choose `D-2a`, should we bring in `exeldro/obs-shaderfilter`'s history by adding `https://github.com/exeldro/obs-shaderfilter.git` as a remote (`shaderfilter`) and merging `shaderfilter/master` (`--allow-unrelated-histories`), preserving his 344 commits and all nleseul / Charles Fettinger / Exeldro notices (`R-LIC-2`) while removing `data/examples/` (`R-LIC-6`) in the merge/cleanup commit?
- **Recommendation**: **Yes** — this matches how `obs-playlister` preserved `CodeYan01/media-playlist-source`'s commit history and keeps attribution transparent in `git log` and `git blame`.
