# Tinyking for EX-word (Edelweiss)

- **LEFT/RIGHT**: ground walk and aim while charging.
- **SHIFT hold**: charge; release to jump; full charge auto-launches on tick 18.
  Hold SHIFT alone for a vertical jump. Air steering is disabled.
- **EXE**: recover after a hard landing, or advance speech.
- **Touch** on the central map: aim relative to the king, hold/release to
  jump, using the Python touch debounce rules. Touch outside the map is ignored.
- **EXIT/ACON**: return to the add-in launcher.

## Hardware recovery test

This build pins Edelweiss `2006f6bb3b8dcfe2d51b9d6d7ec17186f0e539df`. The earlier
"generic" syscall-AE memory implementation is **not safe**: A8500 firmware
uses that same scratch memory internally. This corrected loader selects
the known firmware-fingerprinted P1 RAM arena instead.

The ELF loader now caches four 2 KiB file pages while applying relocations.
TinyKing has hundreds of relocations, so avoiding repeated native seek/read
operations should substantially reduce the otherwise long "loading..."
screen. This is a firmware/EX-word smoke test; actual device behavior is
not yet confirmed. If unsupported, the loader should report an unsupported
ROM instead of guessing RAM ownership.

To replace the previous installation, uninstall the old TKING package,
then install the whole new TKING folder, not just `main.edl`. Ensure
`tinyking.d01`, `main.edl` and `tower.pak` reside in the same
installed `_CONTENT` directory. Avoid leaving older copies installed
elsewhere while testing.

## Asset streaming

`tower.pak` is **not embedded in the ELF**. It is committed at the repository root; CMake and GitHub Actions copy it beside `main.edl` into both locale packages. CI checks that both copies exactly match the committed file and fails if the pack is missing.

- One 320×480 P4 RGB565 background (76,800 bytes + 32-byte palette) at a time.
- The current and adjacent collision rooms; the nearby 64-pixel collision band
  is recomputed only when needed.
- The six-bit/tile masks for the current weather screen, the 392-step gust
  cycle, small resident player images, current-room secret covers, and NPC
  speech pages as needed.

## Install

Install an entire generated locale folder (`build/packages/ja/TKING` or
`build/packages/cn/TKING`) using the usual EX-word add-in transfer process.
The important runtime files must end up together:

```text
TKING/_CONTENT/
  tinyking.d01      # resident Edelweiss loader
  main.edl          # native C game
  tower.pak       # external game assets (1.8 MB)
```

The `main.edl` sidecar path is resolved by Edelweiss. If the asset is absent, the game displays an
error.

**D6600 input-fix retest:** This revised loader no longer refuses all EDLs
because of stale touch-screen or keyboard state. The old `-21` launch
error came from the loader, not from any game or template code. The new
runtime only clears the event queue before and after an EDL run.

## Rendering and startup performance

The renderer keeps the 320x320 map square and two HUD side panels. Each
frame sends only the pixel rectangles that changed using gint's exported
`r61523_display_rect()`, **not** a full 528x320 `dupdate()`. When the camera is
stationary, it restores the union of the old/new king sprite from the P4
background, redraws overlays clipped to that region, and transfers just that
small rectangle. Fractional Q8-only movement does not redraw. Charge bars,
height, falls, status and wind redraw independently, only when changed. The
initial frame uses one full update, and camera shifts or room transitions still
redraw and transfer the entire 320x320 map.

The GMPK table of contents is read in 16-entry batches, reducing the 140-entry
pack from roughly 140 separate seeks plus 140 reads to one seek and nine reads.
This is a syscall-count optimization, not a measured duration.

Edelweiss PR #5 is experimental: its same-base ELF relocation shortcut can
help XD-A8500 (`0x8c700000` link and load address), but not XD-D6600, where
Tinyking's ELF must be rebased to `0x8c800000`. The game does not depend on
that draft PR. Profile on physical EX-word hardware before claiming FPS gains.
