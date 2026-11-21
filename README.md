# Tinyking for EX-word (Edelweiss)

- **LEFT/RIGHT**: ground walk and aim while charging.
- **SHIFT hold**: charge; release to jump; full charge auto-launches on tick 18.
  Hold SHIFT alone for a vertical jump. Air steering is disabled.
- **EXE**: recover after a hard landing, or advance speech.
- **Touch** on the central map: aim relative to the king, hold/release to
  jump, using the Python touch debounce rules. Touch outside the map is ignored.
- **EXIT/ACON**: return to the add-in launcher.

## Hardware recovery test

This build pins Edelweiss `db9a86822724519548f88b0b11d8d97225fc22b5`. The earlier
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
