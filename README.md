# FullThrottle64 — ScummVM R2

This revision stops implementing SCUMM ourselves.

It builds the **real ScummVM SCUMM engine** and attempts to launch the official
Full Throttle DOS demo directly as `scumm:ft` on Nintendo 64.

## Why ScummVM 1.6.0 first?

ScummVM 1.6.0 is the last release era that still contains the official Nintendo
64 backend. Full Throttle support is already in its SCUMM engine. We preserve
the engine and replace the old N64 hardware backend with a libdragon backend.

Once the engine is running reliably, newer Full Throttle fixes can be
backported selectively rather than dragging the entire modern ScummVM platform
surface into 8 MiB RDRAM at once.

## Real engine path

Power on
→ libdragon backend
→ `scummvm_main`
→ `scumm:ft`
→ Scumm engine detection
→ SCUMM v7 engine
→ Full Throttle resources
→ scripts / AKOS / iMUSE / SMUSH / INSANE

The ROM intentionally keeps USB/ISViewer diagnostics enabled. These diagnostics
are around the real engine and are expected to remain during bring-up.

## Controller

- Analog stick: mouse
- A: left click
- B: right click
- Start: F5 / ScummVM menu
- L: Escape

## Demo data

GitHub Actions downloads ScummVM's official Full Throttle DOS demo automatically.
No retail data is committed.
