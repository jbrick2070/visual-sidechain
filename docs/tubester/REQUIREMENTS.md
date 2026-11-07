# Tubester -- Requirements

Status: APPROVED 2026-11-07 by the operator, with D-1 to D-7 answered as recommended. Coding phase 1 (PLAN.md section 7) may start. Rows added or changed after this approval await the operator's review.

Tubester turns a Browser source in OBS Studio into a tape deck: a local video file plays with a live speed control and tape-style pitch, ready to mix in OBS at gigs and on streams.

Conventions: `R-*` rows are requirements, hard unless marked soft. `F-*` rows are facts measured on the date given. `D-*` rows are the operator's decisions. PLAN.md section 7 holds the build steps and section 10 the full measurements.

## 1. Identity and scope

| ID | Requirement |
|----|-------------|
| R-ID-1 | Tubester is part of the Visual Sidechain plugin for OBS 33 or later on Windows (operator's decision, 2026-11-07, replacing D-1). It lives in `C:\visual-sidechain` under `src/tubester/`, with its own tests, so it could be split out again. It is not a fork of OBS and not a separate app. Visual Sidechain's own rules (its REQUIREMENTS.md sections 1, 2, 10, 11, 11a and 12) apply to every Tubester file. |
| R-ID-2 | Name "Tubester". The word "OBS" stays out of the name, icon and identifiers. |
| R-ID-3 | License GPL-2.0-or-later. Author Jeffrey A. Brick. Free, for small tinkerers. Two decks is the target. Pro VJ features are out of scope. |
| R-ID-4 | 100 % local: no cloud, accounts, API keys, analytics or update checks. Network traffic is loopback only. No Python in the product, build or tests. |
| R-ID-5 | Rules from Playlister apply: UTF-8 without BOM, no banned placeholder word, the full test suite after every code change, Sonnet 5.5 QA of each step's diff before commit. |

## 2. Tapes

| ID | Requirement |
|----|-------------|
| R-TAP-1 | A tape is any local file the operator chooses. Tubester never plays or downloads from YouTube, and has no Internet Archive client. |
| R-TAP-2 | Tubester forces no rights check: no prompt, gate, label or required record. Rights are the operator's. One short note in the docs says so. |
| R-TAP-3 | Any legal Windows path works, including spaces, apostrophes, `%`, `#`, `&` and non-Latin letters. |
| R-TAP-4 | The docs list which formats play (F-3) and give one FFmpeg line to convert the rest, including DXV. No built-in converter in 0.1. |

## 3. The deck

| ID | Requirement |
|----|-------------|
| R-DCK-1 | One "Tubester Deck" filter on a normal Browser source makes that source a deck. The filter refuses any other parent type and shows why. |
| R-DCK-2 | Settings: tape, speed, tape pitch on or off, loop, start position. |
| R-DCK-3 | Setup is automatic. The filter sets the page and the Browser source settings the deck needs. The operator never types a port, token or address. |
| R-DCK-4 | A deck is ready before it is first shown. A deck in a hidden scene loads its tape, plays silently and keeps its place, so the first Cut or Fade to it shows moving picture. |
| R-DCK-5 | A hidden deck adds no sound to the mix. |
| R-DCK-6 | Decks are independent. A command to one deck never reaches another. |
| R-DCK-7 | Deck states, shown from what the player reported and never assumed: Ready, Playing, Paused, Ended, Cannot play this file, No picture in this file, No connection. |
| R-DCK-8 | A deck that is playing but has not moved for one second recovers by itself with a seek to its current place, and logs that it did. |

## 4. Speed

| ID | Requirement |
|----|-------------|
| R-SPD-1 | Speed runs from 25 % to 800 % and changes without a restart, a seek or a dropout. |
| R-SPD-2 | With tape pitch on, pitch follows speed exactly. With it off, pitch holds. |
| R-SPD-3 | Fast control input is merged so a turned Knob cannot flood the player. The last value always wins. |
| R-SPD-4 | 1600 % stays out of 0.1, because it froze (F-5). Reverse is out of 0.1. |

## 5. Control and link

| ID | Requirement |
|----|-------------|
| R-CTL-1 | Speed and transport work from OBS hotkeys and from obs-websocket `SetSourceFilterSettings`, so any existing controller can reach a deck. |
| R-CTL-2 | Commands go only to the deck's own source with `javascript_event`, as JSON from one tested writer. |
| R-LNK-1 | The page has no port, token or deck id in its address. The filter sends them as a repeating hello, about once a second for as long as the deck exists, because a page reload gives the plugin no other signal. |
| R-LNK-2 | The page reports with POST as `text/plain` to a listener on `127.0.0.1` on a port the system picks. Reports without the token are dropped. |
| R-LNK-3 | Reports carry page incarnation, load generation and sequence. Old commands and old reports are ignored, so the newest tape always wins a rapid change. |
| R-LNK-4 | After a refresh the deck comes back by itself with its tape, speed and position. Save and reopen brings back the deck and its settings. |
| R-LNK-5 | Removing a deck or its filter in the middle of commands causes no crash or hang. |

## 6. Works with the other tools

| ID | Requirement |
|----|-------------|
| R-INT-1 | Deck audio reaches the OBS mixer as normal source audio, so Visual Sidechain and OBS audio filters work on it. |
| R-INT-2 | Other video filters can sit on a deck. |
| R-INT-3 | Soft: Tubester's filter settings keep stable names from 0.1, for Fadester, the TourBox bridge and the demo kit. |

## 7. Facts measured 2026-11-07

Test copy of OBS 33.0.1 on this PC. F-2 and F-4 used a scratch Lua filter standing in for the planned C filter, so C threading and lifetime remain to be proved in code.

| ID | Fact |
|----|------|
| F-1 | Two 1080p decks on screen use about 0.2 of one CPU core across OBS and its browser processes, on this 24-thread PC. Thirty minutes at two decks skipped 0 of 108,665 rendered frames. |
| F-2 | `javascript_event` through a filter reaches only that source. No crosstalk, and a hello linked in about 250 ms. |
| F-3 | Plays with picture and seeking: H.264 in MP4, MKV and MOV, including 10-bit and 4:2:2; VP9 WebM; AV1 MP4; HEVC MP4 (may depend on this PC's hardware). Synthetic test files, not real Archive downloads. Sound plays from AAC, MP3 and Opus. AC-3 gives picture and silence. |
| F-4 | Files that fail: MPEG-2 and MJPEG AVI give error 4, as does DXV. **MPEG-4 Part 2, Theora and ProRes play sound with no picture and still report "playing"**, so a deck must check for a picture (R-DCK-7). |
| F-5 | The ladder of 0.25x, 1x, 2x, 4x, 8x, 16x, 1x froze in 3 of 8 rounds, all early. readyState stayed 2. The seek recovered all 3 within 1.5 s. A freeze at 16x earlier was also recovered, so 4 of 4 in all. 30 jumps to 8x and 20 to 16x with seeks between showed no freeze, but those positions were already read, so 8x is not proved free of it. R-DCK-8 stays required. |
| F-6 | A never-shown deck runs its page but its video never loads: position 0, readyState 0. Keeping the source showing from the filter with `obs_source_inc_showing` made it play silently: peak 0 in the mix, picture present, position moving. |
| F-7 | 20 tape changes 50 ms apart ended on the last tape with no errors, and an older command was ignored. A missing file gave error 4, and the next good tape played. |
| F-8 | After a refresh, a one-second hello relinked in about 2 s, and the tape resumed 151 ms after that. Switching scene collections away and back kept the filter and the link. |
| F-9 | Ten removals of a filter or its source during a stream of 1,300 to 1,900 commands caused no crash. Commands simply began to fail. |
| F-10 | Visual Sidechain 0.6.0 on a deck: its filter rendered, 0 of 2,334 frames skipped. Level followed the deck at 0.5x to 8x and fell to near 0 when muted. Kick counted 10 times in 5 s at 1x and 0 at other speeds with one rough threshold, on one tape, so its beat follows tape speed poorly. |

## 8. Decisions for the operator

| ID | Decision | Recommendation |
|----|----------|----------------|
| D-1 | Where Tubester lives | Changed 2026-11-07: the operator chose to blend it into the existing Visual Sidechain repository rather than a new one (R-ID-1). Filter id `tubester_deck_filter`, display name "Tubester: Deck" |
| D-2 | The Knob. You chose Tubester before TourBox 0.2.0, so 0.1 is done on hotkeys and scripted Knob-style ramps. Real Knob feel moves to TourBox 0.2.0 and needs a small bridge change there. Accept that "done" for 0.1? | Yes |
| D-3 | YouTube tape type | Drop it. It breaks R-TAP-1, stops at 2x and cannot be used at gigs. VJster already covers private YouTube |
| D-4 | OBS fork | Drop it |
| D-5 | Portable OBS bundle | Drop it for now; ship the plugin alone |
| D-6 | The 58 ms audio delay: leave it to OBS's Sync Offset with a line in the docs, or set it per deck? | Docs line in 0.1. The fix itself is untested |
| D-7 | Speed cap of 800 % for 0.1 | Yes |

## 9. Still unproved

- Real Knob feel (D-2).
- The filter in C: threads, lifetime, and `obs_source_inc_showing` paired with a release on removal.
- Whether a warmed deck holds through long hidden periods. Tested for seconds only.
- A/V sync with the offset applied.
- Real Internet Archive files.
- Other PCs, especially without hardware decoding. Mac and Linux are out of scope for 0.1.
- SmartScreen behaviour for the DLL.
- Reverse.
