# FullThrottle64 — Real ScummVM R3

R3 fixes the demo preparation failure in R2 and continues the real ScummVM
bring-up.

## R2 failure fixed

The official demo can extract `FT.000`, `FT.001`, and `MONSTER.SOU` directly
into the destination directory. R2 then attempted to `cp` each file onto itself.
GNU `cp` returns an error for that operation and `set -e` stopped the Action
before the ScummVM compiler was ever reached.

R3 compares canonical source/destination paths before copying.

## Diagnostics

R3 always tries to preserve:

- `prep.log` — demo download, extraction, validation, ScummVM checkout
- `build.log` — libdragon cross-compile/link
- the N64 backend overlay and build scripts

So the next failure should tell us exactly which real ScummVM compile/link issue
needs fixing.
