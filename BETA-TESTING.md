# Visual Sidechain -- Beta Tester Guide (OBS Studio 33, Windows x64)

Thank you for beta testing **Visual Sidechain**! This single-page guide takes about **10 minutes** in OBS Studio 33 and walks through installing the plugin, setting up the photosensitivity safety hotkeys, testing the 11 built-in FFT-reactive GPU shaders, testing sibling-filter sidechaining with the **Audio Modulator**, and filing your hardware report.

- **Project & Docs (`main`):** https://github.com/jbrick2070/visual-sidechain
- **Latest Release Downloads:** https://github.com/jbrick2070/visual-sidechain/releases/latest
- **Submit Beta Feedback / Bug Report:** https://github.com/jbrick2070/visual-sidechain/issues/new?template=beta-feedback.yml

---

## Before You Start (Requirements & Photosensitivity Safety)

- **Required OBS Version:** **OBS Studio 33.0.0 or newer** on **Windows 10 / 11 (x64)**. Visual Sidechain uses OBS 33's unified plugin package layout (`plugins\visual-sidechain\`) and will not load in OBS 30-32.
- **Photosensitivity Warning:** Audio-reactive shaders (especially `FFT Strobe & Flash`) can produce rapid brightness, contrast, or pattern changes.
  - `FFT Strobe & Flash` defaults to a **3.0 Hz** maximum flash rate cap per photosensitivity guidelines.
  - Before testing on live or high-contrast footage, open **`Settings -> Hotkeys`**, filter for **`Visual Sidechain`**, and bind:
    - **`Visual Sidechain: Panic`** — immediately puts every `Visual Sidechain: Shader Filter` (including custom user shaders) into unmodified pass-through and zeroes all strobe, flash, and modulation amounts across OBS.
    - **`Visual Sidechain: Restore`** — exits pass-through and restores your pre-panic amounts.

---

## Step 1 -- Download & Install (2 min)

Close OBS Studio, then choose the package from [GitHub Releases](https://github.com/jbrick2070/visual-sidechain/releases/latest) that matches how you run OBS 33:

| Your OBS 33 Setup | File to Download | Where to Install |
|---|---|---|
| **Standard Windows Install** (easiest) | `visual-sidechain-*-windows-x64-Installer.exe` | Run the installer (installs to `C:\ProgramData\obs-studio\plugins\visual-sidechain\`). |
| **Standard Windows Install** (manual zip) | `visual-sidechain-*-windows-x64.zip` | Extract so you have `C:\ProgramData\obs-studio\plugins\visual-sidechain\visual-sidechain.dll` and `data\`. |
| **Portable OBS Studio 33** | `visual-sidechain-*-windows-x64-Portable.zip` | Extract directly into your portable OBS 33 root folder so you have `<OBS 33 root>\plugins\visual-sidechain\visual-sidechain.dll`. |

> **Unsigned Binary Note (Windows SmartScreen):** Beta binaries are not Authenticode code-signed yet.
> - On the `.exe` installer, Windows SmartScreen will show *"Windows protected your PC"*; click **More info -> Run anyway**.
> - On either `.zip`, right-click the downloaded `.zip` -> **Properties -> check "Unblock" -> OK** before extracting.
> - *(Note: Windows 11 machines with **Smart App Control** set to strict enforcement block all unsigned third-party DLLs by OS policy.)*

**Verify load:** Start OBS Studio 33, open **`Help -> Log Files -> View Current Log`**, and confirm you see:
```text
[visual-sidechain] plugin loaded successfully (version 0.5.1)
[visual-sidechain] Registered obs-websocket vendor 'visual-sidechain' (get_signals=1)
```

---

## Step 2 -- Test `Visual Sidechain: Shader Filter` & Panic Hotkeys (3 min)

1. Play any music track or drum loop in OBS that routes to **Track 1** (Desktop Audio, a Media Source, or an Application Audio Capture source).
2. Right-click a video source (Webcam, Media Source, Game Capture, or Scene) -> **`Filters`** -> under **Effect Filters**, click **`+`** -> **`Visual Sidechain: Shader Filter`**.
3. Leave **`Audio Input Source`** on **`Track 1 (Main Mix - Default)`** and step through the **`Built-in Shader Preset`** dropdown:
   - `FFT Spectral Edge / Border Aura`
   - `FFT Wave Warp & Glitch`
   - `FFT Chromatic Prism & Bloom`
   - `FFT Visualizer Overlays` *(try switching the `Overlay Mode` dropdown between Spectrum Bars, Oscilloscope, and Radial Ring)*
   - `FFT Zoom Pulse & Barrel Lens`
   - `FFT Kaleidoscope`
   - `FFT Feedback Trails`
   - `FFT Strobe & Flash`
   - `FFT Mirror & Split` *(try switching the `Mirror Mode` dropdown)*
   - `FFT Hex Pulse Lattice` *(try cell density and overlay opacity; bass expands the outlines)*
   - `FFT Polygon Tunnel` *(try three, six, and eight sides, then vary depth and travel speed)*
4. Press your **`Visual Sidechain: Panic`** hotkey while music is playing:
   - Confirm the video immediately drops back to clean, unmodified pass-through.
   - Press **`Visual Sidechain: Restore`** and confirm the shader resumes at its prior intensity.
5. Under **Fader Control**, assign a fader to a geometric parameter and check that its summary matches the target and range. Reset that assignment and confirm the parameter returns to its slider value. A second assignment cannot select a target already owned by the first. Test on a source with transparency and confirm the new geometric overlays stay within the visible source.

---

## Step 3 -- Test `Visual Sidechain: Audio Modulator` & Scene-Save Baseline Safety (3 min)

1. On the same video source, add any standard OBS filter above or below—for example **`Color Correction`** (or, on an NVIDIA RTX GPU with the NVIDIA Broadcast SDK installed, **`NVIDIA Blur`**, **`NVIDIA Background Blur`**, or **`NVIDIA Background Removal`**).
2. Set a recognizable baseline value on that target filter (for example, set `Color Correction -> Saturation` to `0.00` or `Contrast` to `0.25`).
3. Click **`+`** -> **`Visual Sidechain: Audio Modulator`**.
4. In **Slot 1**:
   - Check **`Enable Slot 1`**.
   - Pick your **`Target Filter`** (e.g. `Color Correction`) and **`Target Setting`** (e.g. `Saturation` or `Contrast`).
   - Confirm **`Minimum Output`** and **`Maximum Output`** auto-populate to sensible bounds for that setting, then adjust **`Minimum` / `Maximum`** and **`Amount (%)`** so you can clearly see the filter pulsing to **`Kick Onset`** or **`Bass (60-250 Hz)`**.
5. **Baseline safety check:**
   - Toggle the eye icon next to `Visual Sidechain: Audio Modulator` off (disable it) -> open `Color Correction` and confirm your exact baseline value was restored.
   - Re-enable `Visual Sidechain: Audio Modulator` while audio is actively pulsing, **close OBS completely**, and **reopen OBS** -> confirm your scene saved your original baseline rather than a mid-beat spike.

---

## Step 4 -- Optional Checks (If Applicable to Your Rig)

- **Audio Routing & Ghost Sidechain:** Switch **`Audio Input Source`** to **`Specific Audio Source`**, pick a muted drum/click track, and enable **`React When Source Is Muted (Ghost Sidechain)`** so visuals react to an inaudible trigger track. Also try selecting **`Parent Source Audio`** on a non-audio source (like a Color Source or Image) to confirm the inline warning banner appears cleanly in Properties.
- **Audio-to-Video Sync Offset (`-200..+500 ms`):** If your audio monitoring path or capture card adds delay, adjust **`Audio-to-Video Sync Offset (ms)`** until beat hits line up visually with what you hear.
- **NVIDIA RTX 50-Series (Blackwell) Note:** On RTX 50-series GPUs with NVIDIA Video Effects SDK `0.7.6.0`, modulating `threshold` on `NVIDIA Background Removal` updates at full frame rate, whereas modulating `intensity` on `NVIDIA Blur` / `Background Blur` reloads `NvVFX_Load`—set **`Update Rate Divider`** on that slot to **`Every 2 Frames`** or **`Every 4 Frames`**.
- **Midi Fadester & Stream Deck Fadester Pairing:** If you also have [Midi Fadester](https://github.com/jbrick2070/midi-fadester) installed in `plugins\midi-fadester\` (optionally paired with **Fadester for OBS** on Stream Deck +):
  - In **`Visual Sidechain: Shader Filter -> Fader Control (Midi & Stream Deck Fadester)`**, bind any slot (`Crossfader`, `Fader 1..8`, `Master`) to **`Effect Mix Fader (Dry / Wet)`**, **`Audio Reactivity Fader`**, or **`Parameter Fader 1 / 2`** (e.g. `Aura Intensity`, `Prism Dispersion`, or `Spectrum Bar Height`) and move your physical MIDI fader or Stream Deck + dial.
  - In **`Visual Sidechain: Audio Modulator`**, pick **`Depth Fader (Midi / Stream Deck Fadester)`** on an audio-driven slot to scale `Modulation Amount` live from `0–100%` without losing beat reactivity (or pick `Fader: 1..8` directly as `Driving Audio Signal`).
- **`obs-websocket` 5 Vendor API:** Query `CallVendorRequest` (`vendorName: "visual-sidechain"`, `requestType: "get_signals"`) or subscribe to `VendorEvent` (`eventType: "signals_changed"`, capped at 30 Hz) from Streamer.bot, TouchDesigner, or an `obs-websocket` script.

---

## Step 5 -- Send Your Beta Report (2 min)

Open a **Beta Tester Report** issue here:
👉 **https://github.com/jbrick2070/visual-sidechain/issues/new?template=beta-feedback.yml**

Even a quick **"All passed on Windows 11 + RTX 4070 + OBS 33.0.0"** report is hugely helpful! If anything misbehaves, go to **`Help -> Log Files -> Upload Current Log File`** in OBS before closing it and paste the log URL into your issue.
