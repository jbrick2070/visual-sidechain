# Visual Sidechain has moved into Midi Fadester

> **Deprecated.** Visual Sidechain is no longer a separate plugin. Since **Midi Fadester 0.9.0** (November 2026) it is part of **[Midi Fadester](https://github.com/jbrick2070/midi-fadester)**: one plugin, one install, one version. Please install Midi Fadester instead. This repository is kept only for its history and is not updated any more.

## What to do

1. **Close OBS.**
2. Download and run the latest **[Midi Fadester release](https://github.com/jbrick2070/midi-fadester/releases/latest)**: the Windows installer, or the Mac `.pkg` (universal, macOS 13.0 or newer). It needs OBS Studio 33.0.0 or newer.
3. Start OBS. Visual Sidechain's effects are where they always were, under a source's **Filters**: **`Visual Sidechain: Shader Filter`**, **`Visual Sidechain: Audio Modulator`** and **`Visual Sidechain: Tape Deck`**.

The Midi Fadester installer removes this old plugin for you. On Windows it runs the old Visual Sidechain uninstaller first. On a Mac the package deletes the old `visual-sidechain.plugin` from your OBS plugins folder.

## What keeps working

Everything you set up stays as it is: your filters and their settings, Tape Decks, hotkeys (Panic, Restore visuals, Tap Tempo, Lock BPM, Resync Downbeat) and saved scene collections. Fadester for OBS (Stream Deck and OpenDeck) and the Fadester Companion keep working too. The filter names, hotkey names and the `visual-sidechain` obs-websocket vendor did not change.

If Midi Fadester still finds an old copy of this plugin (for example one unpacked from a `.zip` by hand), it leaves Visual Sidechain switched off so the effects are not registered twice. The **Fadester Desk** dock in OBS (Docks → Fadester Desk) then shows a yellow line naming the old file. Remove that file and restart OBS.

## Where things are now

- **Guide:** [Visual Sidechain in Midi Fadester](https://github.com/jbrick2070/midi-fadester/blob/main/docs/VISUAL-SIDECHAIN.md) covers the shaders, the Audio Modulator, Panic and the flash limit, the Tape Deck and the vendor API.
- **Code:** in Midi Fadester under `src/vs`, `data/shaders`, `data/deck.html` and `tests/vs`, with this repository's history.
- **Problems and ideas:** open an issue on [Midi Fadester](https://github.com/jbrick2070/midi-fadester/issues).
- **The old page:** [README-0.8.0.md](README-0.8.0.md) is this repository's README as it was at the last separate release, 0.8.0. The old releases stay on the [Releases](https://github.com/jbrick2070/visual-sidechain/releases) page for history only. Please do not install them next to Midi Fadester 0.9.0 or newer.

## License

GPL-2.0-or-later, as before. See [LICENSE](LICENSE).
