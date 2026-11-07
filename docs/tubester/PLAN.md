# Tubester plan: tape decks for public domain video

Status: version 2, 2026-11-07. Design tests finished and REQUIREMENTS.md approved the same day. Coding phase (section 7)
under way: step 1, the core, is done; step 2, the Deck filter and its page, is in review. Version 1 planned YouTube decks for private use. The operator
then chose gigs and streams, using public domain and Internet Archive videos, so this version replaces it. The first
reviews are in `reviews/`.

## 1. The idea

Video files load like tapes onto decks, each deck with its own live speed, mixed in OBS Studio and driven from the
Stream Deck, OpenDeck and TourBox. It should feel like a VCR with a variable speed control.

**Who it is for:** small tinkerers. Commercial VJs will pay for Resolume, and Tubester does not compete with it. So it
stays free, simple and light on a home PC. Two decks is the target, and pro features are out of scope.

## 2. The source rule

**Tapes are local files.** Tubester plays files on the operator's disk and never plays YouTube.

- Three kinds of tape, as the operator set them: public domain, fair use, and each person's own videos.
- A public domain film watched through YouTube is still use of YouTube's Service, and its Terms bar public screening.
  So gig tapes come from the Internet Archive or another source that offers the file, not from YouTube.
- Your own videos means your original files. If the only copy is your own YouTube upload, YouTube Studio's download
  should be the route, to be confirmed. Fair use does not change this, because YouTube's Terms are a contract.
- VJster stays as it is for the operator's private use of YouTube in Chrome.
- A tape is any local file the operator chooses. In the first phases the operator gets tapes by hand.
  Tubester adds no Internet Archive client and no bulk download.

## 3. Rights: what stays the operator's job

Leaving YouTube removes YouTube's rules, not copyright.

- Fair use is a defence judged case by case, mainly under US law. Many countries have narrower rules or none. A
  transformed mix helps the argument but does not settle it, and paid use weighs against it. Tubester cannot certify
  it. A platform's automatic matching ignores it.
- Your own videos are safe only for what you own. Music, footage or people in them can carry separate rights.
- The operator's AI renders count as their own videos and are the first tapes. Check each model's licence for public
  or paid use, and any input images or audio. Copyright in pure AI output is unsettled, which limits stopping others
  copying it, not playing it.

- The Internet Archive "does not make guarantees as to the copyright status of items", and use is at the user's own
  risk (its Rights help page). Rights notes usually come from whoever uploaded the item.
- Check each tape's rights: a public domain mark or a licence that allows public, and if paid, commercial use.
  Many Archive items are marked non-commercial and with attribution, which paid gigs may not meet.
- Sound can carry separate rights from the picture. A public domain film may contain a protected recording.
- Public domain status varies by country.
- Streaming platforms' automatic matching can still flag public domain material, and disputes take time.
- Prelinger Archives is a mixed pool. Its current FAQ, as Codex reported it, says many films are copyrighted or orphan
  works, so a collection cannot be assumed public domain.
- Attribution licences such as CC-BY need a credit the audience can see. Musical compositions, recordings shown in a
  stream or VOD, and likeness or trademark rights are separate again.
- Unverified: the Internet Archive Terms of Use page would not load as text here. agy reports it still says access
  is "for scholarship and research purposes only". A 2014 Archive post says the noncommercial certification was removed
  and use must be noninfringing or fair. Read the current page before relying on either.

**Tubester forces no rights check.** Operator's decision, 2026-11-07: rights are up to the user. Tubester plays any
local file, with no prompt, gate, label or required record. This section is guidance for one short docs note, and is
not legal advice.

## 4. The player: a video element in a Browser source

| Route | Live speed | Result |
|---|---|---|
| OBS Media Source or Playlister | No. `speed_percent` is 1 to 200, and any change restarts the media (`obs-ffmpeg-source.c:439-441`) | Rejected for the Knob. |
| `<video>` in a normal Browser source | `playbackRate` changes in place per Chromium's report; not yet tested in OBS | **Chosen for the first proof**, because a no-plugin test takes very little work. |
| Native deck: our own `OBS_SOURCE_TYPE_INPUT` with FFmpeg | Full control, native media controls and hidden-scene playback, no browser or HTTP | **Named fallback.** agy's preferred route. More code, with audio, sync and speed all ours to write. Taken if a kill gate in section 6 fails. |

Why `<video>` suits a VCR feel:
- Setting `preservesPitch` to false lets the pitch fall and rise with speed, like tape.
- Speed is not capped at 2x by YouTube. Chromium's limit is 0.0625x to 16x, and with `preservesPitch` off the audio is
  resampled across that range (reviewers read this in Chromium source). Usable range still needs measuring.
- No ads, no embed blocks, no API rules, and it works with no network at a gig.
- Reverse does not exist as a negative rate. All three reviewers doubt stepping `currentTime`: it has no sound and
  flushes the decoder every frame. So reverse means a pre-rendered reversed copy of the tape, swapped in on a direction
  change. It is optional.

Formats: OBS's browser ships H.264 and AAC on Windows (maintainer statements). agy reports no MPEG-2, Theora, `mp4v`
or ProRes, so some Archive files and master renders need converting to H.264 first.

## 5. Shape

The first reviewers' reasons still hold: a wrapper source has to rebuild audio, activation, colour and interaction.
So the plugin is a **Tubester Deck filter on a normal Browser source**. It holds the tape, speed and transport
settings, and sends commands with `javascript_event`. A wrapper comes later only if media controls are missed.

Forking OBS and a native app stay rejected.

## 6. Design phase 1: no product code

Everything runs in portable OBS 33 on a test scene collection, never the operator's OBS or password.

| Step | Question | Proposed pass |
|---|---|---|
| 1 | Operator picks tapes: their own AI renders first, then Archive files. Steps 2 onward start with whatever is ready | At least 3 files, covering render and Archive formats |
| 2 | **Kill gate.** Does OBS's browser play them with seeking? Try `file:///` first, because `http://absolute/` has no Range support. A small local file server is the last resort | Clean frames, sound and seeking |
| 3 | **Kill gate.** With `reroute_audio` on, does sound reach the OBS mixer in sync? Does a Fade between scenes show black frames, and does a hidden deck keep its place? | No black frames; drift under one frame a minute |
| 4 | **Kill gate.** 2 decks at 1080p for 30 minutes, then 4 for information | Under 0.5% dropped frames with 2 decks |
| 5 | Speed feel: stall, pitch, usable range, 50 ms coalescing, with a real Knob driver | No audible restart; range set from measurement |
| 6 | Filter, ready handshake and return path together, with recovery: refresh, save and reopen, missing file, rapid tape change, deletion mid-message, two decks independent | No hang, crash or crosstalk |
| 7 | Optional: reversed-copy swap | Operator says it feels usable, or it is dropped with no effect on the rest |

Any failed kill gate moves the design to the native deck, using the same tapes, core and hardware route.

Report after each step. Exit: a go or no-go with numbers, then REQUIREMENTS.md in VJster's style,
approved by the operator.

## 7. First coding phase: one deck, with honest feedback

Goal: one Browser source becomes one tape deck, with speed and transport from OBS and hardware, reporting only what
the player confirmed.

**Repository:** operator's decision 2026-11-07: inside Visual Sidechain (`C:\visual-sidechain`), as `src/tubester/`
with `tests/test-tubester-*.c`, using that plugin's build, CI, formatters and locale checks. The earlier plan of a new
`jbrick2070/tubester` repository is dropped. A copy of this plan and REQUIREMENTS.md lives there in `docs/tubester/`.

**Parts:**
- Pure C core, unit tested without OBS: path checks, rate clamp and step, report parser, generation check, JSON writer.
- `tubester_deck_filter`: settings `tape`, `speed_percent` (range from step 5), `tape_pitch`, `loop`,
  `start_seconds`. One filter per source, attached only when the parent is a `browser_source`.
- Parent ownership: hold an `obs_weak_source_t` from `filter_add` to `filter_remove` and promote it per call. Never
  keep a strong reference or call `obs_filter_get_parent` outside its allowed callbacks.
- Parent setup: page address (`is_local_file` off), `reroute_audio` on, `shutdown` off, `restart_when_active` off.
  Checked when the deck claims its source and after each update signal from the source, never on a timer, on the UI
  thread where the source's Properties dialog writes the same settings, and changed only where they differ. After a
  change nothing goes to the page until the source has applied it. Never call `obs_source_update(parent, NULL)`,
  because that reloads the page. Tape changes go through `javascript_event`, not the parent's settings. One deck drives
  a source; a second one waits and takes over when the first is removed.
- Events: one constant event name, and JSON from the C writer. Event names are spliced unescaped into page code, and bad
  JSON can break the event or the page process, so a Windows path is escaped before it is sent.
- Control: `SetSourceFilterSettings` over obs-websocket and hotkeys, through one serialized setter on a worker thread.
- Return path: the plugin sends a repeating hello event carrying port and token until the page answers, so neither sits
  in a URL, the scene file or the logs. The page reports with POST to a listener on `127.0.0.1:0`, as `text/plain` to
  avoid a CORS preflight.
- Handshake runs per page load. Reports carry deck id, page incarnation, load generation and sequence, so stale ones are
  ignored. Removing a deck unregisters it under a lock. The shared listener stops at module unload; a deck destroyed
  after that unregisters under a lock that is never destroyed and finds nothing.
- `deck.html`, one file, no build step: `<video>`, rate, pitch, loop, states.
- Deck states: Ready, Playing, Paused, Ended, Cannot play this file, No connection.

**Steps, each with Sonnet 5.5 QA of its diff before commit:**
1. Skeleton, CI, core with tests.
2. Filter loads a tape; `deck.html` plays it.
3. Listener, hello handshake, honest states. The listener and the handshake landed in step 2. Honest states still
   to do: compare the rate, tape and position the page reports with what the deck asked for.
4. Speed and pitch: settings, hotkeys, coalescing, replay.
5. Transport. A two-deck independence check. Also a Cue action (go to the start position now; changing the start
   position only applies at the next tape load), and a page reload must keep a paused deck paused (the tape command
   plays at once today).
6. Knob route: a separate commit in the Fadester repository so the bridge sends speed to a deck.
7. Codex review of the whole, then the operator's hardware test.

**Out of this phase:** reverse, Internet Archive client, tape library, dock panel, wrapper source, Stream Deck actions,
releases, translations, any listing.

**Done means:** the operator picks a file, presses play, turns the Knob, and the picture and pitch change on OBS
Program with no restart. Nothing asks for a port or token.

## 8. Later: the look

After the first deck works, a Tubester panel inside OBS gives it the VCR feel:
- Each tape is a drawn VHS cassette with a handwritten-style label: the title, or the link it came from.
- Drag a tape onto a deck to load it, with an insert animation.
- A tape counter, chunky transport buttons, and a shuttle dial for speed.
- Optional VCR-style on-screen text and tracking lines, as OBS filters on the file decks.

A YouTube link can sit on a tape as a separate tape type, played through the embedded player: 2x at most in OBS, and
YouTube's Terms apply, so it is for private use. Nothing may be drawn over that player, so the art stays in the panel
and the on-screen effects stay off those decks. Tubester never downloads from YouTube.

## 9. Open risks

- CEF has had multi-video regressions (obs-browser issue 470), which step 4 tests.
- A `file:///` page may be blocked from posting to loopback, which step 6 tests.
- A filter cannot use `TriggerMediaInputAction`, so existing transport buttons do not reach decks until the bridge
  learns them.
- A DLL loaded by OBS is expected to avoid SmartScreen. Unverified.
- The OBS forum bars mostly AI-written resources and "OBS" in a brand, so assume no forum listing.
- The hello carries the token to whatever page the Browser source shows. After the deck changes the source's address
  it waits for the source's update, which covers a source that showed another page. A browser for an old address still
  being created at that moment could in theory get the hello (step 2 review). A later hardening could let only a
  local file page obtain the token.

## 10. Design phase results, 2026-11-07

Measured in a separate test copy of OBS 33.0.1, on real files, with a scratch page. The scripts are in `probes/`.

| Check | Result |
|---|---|
| Route | `file:///` page and tape. No file server needed. Start 0.1 to 0.6 s, seek 51 to 152 ms, a space in a path works |
| `http://absolute/` | Rejected. Chromium blocks its reports to loopback, while a `file://` page can POST |
| Formats | Operator's AI renders are H.264 and AAC and play. DXV fails cleanly with error 4. FFmpeg converts DXV without Resolume (2 s sample) |
| Mixer audio | Reaches the OBS meter. A hidden deck adds no sound to the mix |
| Sync | Audio about 58 ms late, drift 2 ms in 70 s. Fix with OBS's sync offset or in the deck |
| Fade and Cut | No black frames in Program. A first-shown deck starts within 40 ms |
| Hidden decks | Keep position and loop once shown. A never-shown deck runs its page, but its video does not load. `obs_source_inc_showing` from the filter fixes that, silently |
| 2 decks, 30 min, one at 1.5x, recording | 0 skipped of 108,665 rendered frames, 0 of 108,588 output frames, 4 dropped of 67,996 video frames |
| 4 decks, 3 min | 0 skipped of 10,854 frames, GPU about 6% |
| Speed 0.25x to 16x | Measured rate within 0.15%. Change arrives in about 50 ms |
| Pitch | Tape-like and exact from 0.25x to 8x: a 1 kHz tone reads 250 Hz to 8 kHz |
| Knob-style ramp | 160 changes in 8.5 s, no dropout, stall or seek |
| 16x | **Froze twice** on a jump to 16x, position stuck and readyState 2. It stayed stuck at 1x. A seek to the current place recovered it the one time that was tried. Six other 16x runs worked |

Decisions from these results: cap speed at 8x for now. The deck needs a watchdog: if a playing deck's position stops, seek to the current place. Add a one-time sync offset.

All three kill gates pass, so the browser route stands and the native deck stays unused.

The second round, the same day, covered step 6 with a scratch Lua filter, plus formats, CPU and Visual Sidechain. The numbers are in REQUIREMENTS.md section 7 and the scripts are `gate6*.mjs` and `gate7.mjs`. Every check passed, with these changes to section 7 below:

- The filter keeps its source showing, with `obs_source_inc_showing`, so a hidden deck is ready. It releases that on removal.
- The hello repeats for the life of the deck, about once a second, because a page reload gives the plugin no other signal.
- The deck adds a state, No picture in this file. MPEG-4 Part 2, Theora and ProRes play sound with no picture and still report "playing".
- The watchdog is required. The 16x ladder froze in 3 of 8 rounds, and the seek recovered all 3.

Not yet done: speed feel on a real Knob, reverse copy, and the items in REQUIREMENTS.md section 9.

## 11. Development order

Fable and Gemini answered separately and agreed on the main order.

1. **TourBox 0.2.0 first.** The Knob route lives in that repository, and the real-Knob test needs a working bridge. Depends on the operator's rc3 hardware checks.
2. **Close the design phase:** real Knob feel, then REQUIREMENTS.md. A scripted filter and recovery proof follows.
3. **Tubester 0.1:** one deck in code, section 7 steps 1 to 5. Forced by step 2.
4. **Hardware:** Knob route in the TourBox bridge, then Fadester speed and transport on Stream Deck and OpenDeck.
5. **DXV tape converter.** FFmpeg by hand covers this until then.
6. **Demo kit** with Visual Sidechain and Playlister. It needs final setting names.
7. **VHS tape panel,** then Midi Fadester and the APC40.

Both reviewers say to drop the OBS fork and the YouTube tape type, and Gemini also drops the portable bundle. Those stay open as the operator's decisions. Reverse and a wrapper source wait.

**Operator's decision, 2026-11-07: stay on Tubester rather than finish TourBox 0.2.0 first.** So the design phase closed without a real Knob. Item 3 comes next, once REQUIREMENTS.md is approved, and the real Knob test moves to TourBox 0.2.0 (D-2 in REQUIREMENTS.md).
