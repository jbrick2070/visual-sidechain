# Changelog

All notable changes to Visual Sidechain are listed here, newest first. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the version numbers are the ones in `buildspec.json`.

## [Unreleased]

## [0.7.7] - 2026-11-18

### Changed

- **Version only:** no changes to the plugin. The number follows Midi Fadester 0.7.7 (APC40 home blends and lit animations), as the two share one version.

## [0.7.6] - 2026-11-18

### Changed

- **Version only:** no changes to the plugin. The number follows Midi Fadester 0.7.6 (a music file no longer takes an APC40 fader), as the two share one version.

## [0.7.5] - 2026-11-18

### Changed

- **Version only:** no changes to the plugin. The number follows Midi Fadester 0.7.5 (the APC40 mkII as a whole VJ deck), as the two share one version. Midi Fadester's Record, Metronome and Shift + Tap Tempo now call Visual Sidechain's existing `panic` / `restore`, `tap_tempo`, `lock_bpm` and `resync_downbeat` requests.

## [0.7.4] - 2026-11-18

### Changed

- **Version only:** no changes to the plugin. The number follows Midi Fadester 0.7.4 (video speed on the APC40 mkII's transport controls), as the two share one version.

## [0.7.3] - 2026-11-18

### Changed

- **Version only:** no changes to the plugin. The number follows Midi Fadester 0.7.3 (its APC40 mkII VJ pads), as the two share one version.

## [0.7.2] - 2026-11-17

### Added

- **macOS build:** a universal (Apple Silicon and Intel) plugin for OBS Studio 33 on macOS 13.0 or newer, built and tested in CI on the `macos-26` runner and shipped as `visual-sidechain-<version>-macos-universal.pkg` and `.tar.xz` next to the Windows files. Unsigned and not notarized; the README says how to open it. It has every feature of the Windows build. On 2026-11-17 it was installed in OBS 33.0.1 on an Apple Silicon Mac (M6, macOS 27.3) and checked: it loads, its vendor requests answer, all 11 shaders compile on the Mac GPU and behave as on Windows, Panic, Restore visuals and saved hotkeys work after a restart, and OBS quits with 0 memory leaks. The full hands-on test with controllers is still to come.
- **Tape Deck on macOS:** the report listener runs on POSIX sockets (loopback only, close-on-exec, no `SIGPIPE`, the port held exclusively), tokens come from `arc4random_buf`, and tapes and the deck page take absolute macOS paths (`file:///Users/...`).
- **`test-shader-gpu` (macOS CI):** builds every built-in shader with OBS's own OpenGL (HLSL to GLSL) and Metal renderers and fails on any translation or compile error; a broken shader must be refused.

### Changed

- Built-in shaders now compile on macOS too: six of them used `(float)x` casts or `[unroll]`, which OpenGL's GLSL refuses; they are written as `float(x)` and plain loops, and `check-shader-portability` catches those two forms on Windows as well.
- The listener and token tests and the tape-path tests run on Windows and macOS; the path rules of both systems are checked on both.

## [0.7.1] - 2026-11-16

### Fixed

- Hotkeys set in Settings > Hotkeys (Panic, Restore, Tap Tempo, Lock BPM, Resync Downbeat) are now loaded at startup and on profile change; before, they stopped working after OBS restarted.

## [0.7.0] - 2026-11-15

### Added

- **Suite Start Here link (`P1-5`):** Linked the suite-wide `START-HERE.md` setup and compatibility guide first at the top of `README.md`.

- **Tape Deck vendor control and events (`P1-6`):** Added `visual-sidechain` obs-websocket vendor request `deck_command { deck, op, percent?, steps? }` (`play_pause`, `cue_to_start`, `speed_set`, `speed_step`, `normal_speed`) reusing the `tubester_deck.*` hotkey paths with plain-sentence validation and per-frame `speed_set` coalescing, vendor event `decks_changed` (same payload as `get_decks`, coalesced to at most 30 events/sec without firing on ordinary playback position advances), additive `version` field on `get_decks`, and the `"Which video player when"` guide in `README.md` (`api_version` stays `1`; Tape Decks stay outside Panic and Restore).

- **Tempo foundation (`P1-2`):** Added `src/core/tempo.{h,c}`, `visual-sidechain` obs-websocket vendor requests `tap_tempo`, `set_bpm { bpm }`, `lock_bpm { locked }`, and `resync_downbeat`, OBS frontend hotkeys `visual_sidechain_tap_tempo`, `visual_sidechain_lock_bpm`, and `visual_sidechain_resync_downbeat` across all 12 locales, and 4-beat bar signals `beat_index` (`0..3`), `bar_phase` (`0.0..1.0`), `bar_index` (`>= 0`), and `bpm_locked` (`bool`, default `false`) on `get_signals`, `signals_changed`, `visual_sidechain_get_signals`, and `visual_sidechain_signals` (`api_version` stays `1`). When `bpm_locked` is on, kicks never alter tempo (only gently nudging phase near a beat boundary) and the bar clock keeps running continuously across kickless gaps and silence.

- **Panic truth channel (`P0-3`):** Added `visual-sidechain` obs-websocket vendor requests `get_safety`, `panic`, and `restore`, vendor event `safety_changed` (`{ version, api_version: 1, panic, since_ms }`), and additive `panic` boolean on `get_signals` / `signals_changed`. Both vendor requests and OBS frontend hotkeys (`visual_sidechain_panic` / `visual_sidechain_restore`) emit `safety_changed` and share the UI-thread filter walk (`api_version` stays `1`).
- `audio_present` (`bool`) and `beat_confidence` (`0.0..1.0`) fields on the `visual-sidechain` vendor API (`get_signals`, `signals_changed`) and in-process `visual_sidechain_get_signals` / `visual_sidechain_signals` bridge (`api_version` stays `1`).
- Live snapshot Audio Input Status line (`"Hearing audio on ..."` / `"No audio on ... for N s."`) with an explicit **Refresh Audio Status** button in Shader Filter and Audio Modulator Properties across all 12 locales.

### Changed

- **Suite naming pass (`P1-7`):** Replaced legacy *Stream Deck Fadester* and bare *Fadester* product labels with *Fadester for OBS* and *Midi Fadester* across all 12 locale files (`data/locale/*.ini`), qualified *Restore* as *Restore visuals* (`Hotkey.Restore`, `Panic.Active`, `Shader.PanicHint`), and standardized *Cue to start* (`Tubester.CueToStart`, `Tubester.Hotkey.CueToStart`) in `en-US.ini` without changing any hotkey ID, setting key, or vendor request name.

- **Shared process-wide flash clock (`P0-1`):** Moved flash-rate limiting into a process-wide shared clock (`src/core/flash-clock.{h,c}`) capped at 3 flashes per second across all active Visual Sidechain filters and guarded Audio Modulator slots combined (admitted in arrival order; per-shader `max_flash_rate_hz` can only lower that filter's personal rate).
- **Limited transient uniforms and modulator sources (`P0-1`):** Added `audio_kick_safe`, `audio_snare_safe`, `audio_flux_safe`, and `audio_spectral_flux_safe` shader uniforms plus matching Audio Modulator signal sources (`Kick Onset (Flash-Safe)`, `Snare Onset (Flash-Safe)`, `Wideband Spectral Flux (Flash-Safe)`).
- **Built-in shader flash audit (`P0-1`):** Routed whole-frame luminance, highlight bloom, border/silhouette aura, trail persistence, and edge vignette transients across the 11 built-in shaders onto `vs_flash_envelope` / `audio_kick_safe` while keeping spatial UV geometry on raw transients, enforced by `check-shader-flash-safety`.
- **Audio Modulator Flash Guard & custom shader notice (`P0-1`):** Added per-slot `Flash Guard` (on by default, logged once if turned off) gating transient signals (`Kick`, `Snare`, `Spectral Flux`, `Beat`, and `Custom Hz` flux) on luminance-class target settings (`brightness`, `contrast`, `gamma`, `opacity`, `color_multiply`, `color_add`), and added an inline safety notice next to the custom shader file picker and code editor.

### Fixed

- Beat phase no longer freewheels in silence: after `2.0 s` below the noise gate or without audio callbacks (and before the first above-gate audio), `beat` (`beat_phase`) freezes at `0.0`, `audio_present` reports `false`, `beat_confidence` reports `0.0`, and `signals_changed` stops emitting until audio resumes.
- Corrected stale kick (`55..130 Hz`) and snare (`250..4000 Hz`) frequency band comments in `src/core/analyzer.h`.
- `README.md`: clarify that version `0.6.0` is shared with Midi Fadester while Fadester for OBS is versioned separately (`0.11.0`).

[Unreleased]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.7...HEAD
[0.7.7]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.6...0.7.7
[0.7.6]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.5...0.7.6
[0.7.5]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.4...0.7.5
[0.7.4]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.3...0.7.4
[0.7.3]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.2...0.7.3
[0.7.2]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.1...0.7.2
[0.7.1]: https://github.com/jbrick2070/visual-sidechain/compare/0.7.0...0.7.1
[0.7.0]: https://github.com/jbrick2070/visual-sidechain/compare/0.6.0...0.7.0
