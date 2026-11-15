# Changelog

All notable changes to Visual Sidechain are listed here, newest first. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the version numbers are the ones in `buildspec.json`.

## [Unreleased]

### Added

- `audio_present` (`bool`) and `beat_confidence` (`0.0..1.0`) fields on the `visual-sidechain` vendor API (`get_signals`, `signals_changed`) and in-process `visual_sidechain_get_signals` / `visual_sidechain_signals` bridge (`api_version` stays `1`).
- Live snapshot Audio Input Status line (`"Hearing audio on ..."` / `"No audio on ... for N s."`) with an explicit **Refresh Audio Status** button in Shader Filter and Audio Modulator Properties across all 12 locales.

### Fixed

- Beat phase no longer freewheels in silence: after `2.0 s` below the noise gate or without audio callbacks (and before the first above-gate audio), `beat` (`beat_phase`) freezes at `0.0`, `audio_present` reports `false`, `beat_confidence` reports `0.0`, and `signals_changed` stops emitting until audio resumes.
- Corrected stale kick (`55..130 Hz`) and snare (`250..4000 Hz`) frequency band comments in `src/core/analyzer.h`.
- `README.md`: clarify that version `0.6.0` is shared with Midi Fadester while Fadester for OBS is versioned separately (`0.11.0`).
