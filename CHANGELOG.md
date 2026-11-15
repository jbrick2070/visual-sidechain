# Changelog

All notable changes to Visual Sidechain are listed here, newest first. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the version numbers are the ones in `buildspec.json`.

## [Unreleased]

### Added

- **Tempo foundation (`P1-2`):** Added `src/core/tempo.{h,c}`, `visual-sidechain` obs-websocket vendor requests `tap_tempo`, `set_bpm { bpm }`, `lock_bpm { locked }`, and `resync_downbeat`, OBS frontend hotkeys `visual_sidechain_tap_tempo`, `visual_sidechain_lock_bpm`, and `visual_sidechain_resync_downbeat` across all 12 locales, and 4-beat bar signals `beat_index` (`0..3`), `bar_phase` (`0.0..1.0`), `bar_index` (`>= 0`), and `bpm_locked` (`bool`, default `false`) on `get_signals`, `signals_changed`, `visual_sidechain_get_signals`, and `visual_sidechain_signals` (`api_version` stays `1`). When `bpm_locked` is on, kicks never alter tempo (only gently nudging phase near a beat boundary) and the bar clock keeps running continuously across kickless gaps and silence.

- **Panic truth channel (`P0-3`):** Added `visual-sidechain` obs-websocket vendor requests `get_safety`, `panic`, and `restore`, vendor event `safety_changed` (`{ version, api_version: 1, panic, since_ms }`), and additive `panic` boolean on `get_signals` / `signals_changed`. Both vendor requests and OBS frontend hotkeys (`visual_sidechain_panic` / `visual_sidechain_restore`) emit `safety_changed` and share the UI-thread filter walk (`api_version` stays `1`).
- `audio_present` (`bool`) and `beat_confidence` (`0.0..1.0`) fields on the `visual-sidechain` vendor API (`get_signals`, `signals_changed`) and in-process `visual_sidechain_get_signals` / `visual_sidechain_signals` bridge (`api_version` stays `1`).
- Live snapshot Audio Input Status line (`"Hearing audio on ..."` / `"No audio on ... for N s."`) with an explicit **Refresh Audio Status** button in Shader Filter and Audio Modulator Properties across all 12 locales.

### Changed

- **Shared process-wide flash clock (`P0-1`):** Moved flash-rate limiting into a process-wide shared clock (`src/core/flash-clock.{h,c}`) capped at 3 flashes per second across all active Visual Sidechain filters and guarded Audio Modulator slots combined (admitted in arrival order; per-shader `max_flash_rate_hz` can only lower that filter's personal rate).
- **Limited transient uniforms and modulator sources (`P0-1`):** Added `audio_kick_safe`, `audio_snare_safe`, `audio_flux_safe`, and `audio_spectral_flux_safe` shader uniforms plus matching Audio Modulator signal sources (`Kick Onset (Flash-Safe)`, `Snare Onset (Flash-Safe)`, `Wideband Spectral Flux (Flash-Safe)`).
- **Built-in shader flash audit (`P0-1`):** Routed whole-frame luminance, highlight bloom, border/silhouette aura, trail persistence, and edge vignette transients across the 11 built-in shaders onto `vs_flash_envelope` / `audio_kick_safe` while keeping spatial UV geometry on raw transients, enforced by `check-shader-flash-safety`.
- **Audio Modulator Flash Guard & custom shader notice (`P0-1`):** Added per-slot `Flash Guard` (on by default, logged once if turned off) gating transient signals (`Kick`, `Snare`, `Spectral Flux`, `Beat`, and `Custom Hz` flux) on luminance-class target settings (`brightness`, `contrast`, `gamma`, `opacity`, `color_multiply`, `color_add`), and added an inline safety notice next to the custom shader file picker and code editor.

### Fixed

- Beat phase no longer freewheels in silence: after `2.0 s` below the noise gate or without audio callbacks (and before the first above-gate audio), `beat` (`beat_phase`) freezes at `0.0`, `audio_present` reports `false`, `beat_confidence` reports `0.0`, and `signals_changed` stops emitting until audio resumes.
- Corrected stale kick (`55..130 Hz`) and snare (`250..4000 Hz`) frequency band comments in `src/core/analyzer.h`.
- `README.md`: clarify that version `0.6.0` is shared with Midi Fadester while Fadester for OBS is versioned separately (`0.11.0`).
