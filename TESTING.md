# visual-sidechain -- Testing Checklist

Companion to [REQUIREMENTS.md](REQUIREMENTS.md) and [PLAN.md](PLAN.md).

---

## Phase 1 -- OBS 33 Build, Layout, and Load Smoke Checks

| # | Check | Expected Log Line / Result | Status |
|---|-------|----------------------------|--------|
| 1.1 | Configure & build `visual-sidechain` (`cmake --preset windows-x64` && `cmake --build --preset windows-x64 --config RelWithDebInfo`) | Compiles with 0 warnings (`CMAKE_COMPILE_WARNING_AS_ERROR` ON); `dumpbin /exports` shows `obs_module_load`, `obs_module_unload`, `obs_module_name`, `obs_module_description`. | Passed (2026-10-27) |
| 1.2 | CTest suite (`ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure --no-tests=error`) and mutation check | `check-locale-keys` and `check-locale-keys-fails-on-missing-key` pass (`2/2`); deliberate `MissingKeyProbe` in `src/plugin-main.c` causes `check-locale-keys` to fail non-zero (`exit code 8`). | Passed (2026-10-27) |
| 1.3 | Install layout (`cmake --install build_x64 --config RelWithDebInfo --prefix release/RelWithDebInfo`) | Produces `release/RelWithDebInfo/visual-sidechain/visual-sidechain.dll`, `visual-sidechain.pdb`, `data/manifest.json`, and `data/locale/en-US.ini` (no `bin/64bit`). | Passed (2026-10-27) |
| 1.4 | Portable OBS 33.0.0 load & clean exit (`C:\Users\jeffr\OBS-Studio-33-Portable`) | Copy to `plugins\visual-sidechain\`, launch from `bin\64bit`, close with `CloseMainWindow()`: log `2026-10-27 13-24-25.txt` contains `[visual-sidechain] plugin loaded successfully (version 0.1.0)`, `[visual-sidechain] plugin unloaded`, and `Number of memory leaks: 0`; `plugin_manager/modules.json` records `"display_name": "Visual Sidechain"`, `"version": "0.1.0"` with no `Legacy` flag. | Passed (2026-10-27) |
| 1.5 | Windows security & unsigned DLL (`R-PLAT-10`) | `Get-AuthenticodeSignature` reports `NotSigned` as expected; `visual-sidechain.dll` loads cleanly in OBS 33.0.0 without Defender or Code Integrity blocks. | Passed (2026-10-27) |
