# MAME 0.289 — Atari 2600 / 7800 / 8-bit patch set (unofficial)

An unofficial patch set for MAME 0.289 (tag `mame0289`) that improves the Atari 2600, Atari 7800 and
Atari 8-bit (400/800/XL/XE/XEGS) drivers. It is not a MAME release and is not supported by MAMEdev:
please report problems on this fork, not to MAME. MAME is a registered trademark of Gregory Ember.

Branch `atari-0.289` is MAME 0.289 plus the commits listed by `git log mame0289..atari-0.289`; each
change is its own commit with a description of what was measured.

## Atari 7800 (`a7800`, `a7800p`)

- **2600 mode.** A 2600 cartridge (`-cart2600 game.a26`) runs as on a real 7800. The BIOS finds it,
  runs its 2600 bootstrap and locks INPTCTRL with MARIA off. From then on:
  - the TIA draws the screen;
  - the CPU and the RIOT run at 1.19 MHz;
  - the CPU sees a 6507's 13-bit bus.

  All of MAME's 2600 cartridge types work, including the new ones below, and Harmony/ARM carts too.
  `-bios skip` with a 2600 cartridge starts straight in 2600 mode. Results:
  - **Compared with the `a2600` driver** on 251 USA games: 242 are pixel-identical at 10 s. The other
    9 keep identical RAM; `a2600` uses a 3.579575 MHz clock, the 7800 its own 3.579545 MHz crystal.
  - **Through 7800OpenBIOS:** all 16 games tested boot into their attract modes.
  - **Soft reset and save states** work.
- **BankSet cartridges**, all 11 forms (CPU half and MARIA half, BankSet RAM, 52K halves, POKEY at
  `$4000` or `$0800`), plus the spec's rules: display lists must be in console RAM, and no code may
  run from cart RAM.
- **POKEY at `$0800`**, and **dual POKEY** at `$0440`+`$0450` (both chips at half gain).
- **Activision `.a78` images in the usual block order** now boot.
- **MARIA timing from the 7800-devtools a7800 fork**, with MARIA's DMA in its own address space, so
  debugger and Lua watches can tell its reads from the CPU's.
- **`-bios skip`** boots straight into the cartridge, with no BIOS file needed.

## Atari 2600 (`a2600`, `a2600p`)

- **Frame Blending setting** (Machine Configuration menu, on by default; also used by the 7800's
  2600 mode). MAME blends each frame with the one before so flickering sprites look steady; turn it
  off to see each frame as drawn, e.g. for a sprite that changes every frame.
- **Harmony/Melody ARM cartridges: DPC+, CDF, CDFJ and CDFJ+.**
  - **How:** the game's own ARM code runs on MAME's ARM7; the Harmony driver's behaviour is
    implemented natively.
  - **Tested:** 25 Champ Games/SpiceWare demos, Space Rocks, Stay Frosty 2 (demo and later build),
    Draconian and Frantic boot and play. Their attract screens match Stella's, apart from MAME's TIA
    frame blending.
- **New bank-switching boards:**
  - FA2 (Star Castle Arcade);
  - EF/DF/BF and their Super Chip versions;
  - SB, 3E+, 0840, UA with swapped banks, 0FA0, 03E0 and MDM;
  - E7 with 8K and 12K.
- **Supercharger tape images** (`.a26`) load as cartridges, multiload included. All 8 in the USA set
  reach their title screens.
- **Better detection.**
  - 64K images are no longer all taken as 32-in-1 multicarts.
  - Harmony images are recognised by their driver.
  - F8 games that must start in bank 1 are detected (Congo Bongo).
  - Forcing a board with `-cartslot` allocates that board's RAM.

## Atari 8-bit (`a800`, `a800xl`, `a130xe`, `xegs`, ...)

- **ANTIC DMA cycle by cycle**, as documented in the Altirra Hardware Reference Manual. Stock MAME
  charged the CPU 1/8 of a cycle per stolen pixel, so games ran too fast. Karateka (XEGS) showed
  this: a garbled title line, a fast story crawl and a crash in the attract demo, all fixed.
- **CART (`.car`) headers.** Every type in the Atari800 list is named, more are mapped (XEGS
  64K/128K and others), and the checksum is checked. `.car` files load without stripping the
  header.

## Core and debugging

- **`-debugger script`**: a windowless debugger whose stops hold while Lua runs, so scripts can use
  breakpoints and stepping.
- **`-playback_offset N` / `-playback_force`**: play an `.inp` file on a game that starts on another
  frame.
- **The 6532 RIOT timer is cycle-exact.** It ran one or two cycles early, which broke games that
  poll INTIM for 0 once a line (Stay Frosty 2). A read after the timer runs out now resumes counting
  on the prescaler's own phase. Checked against Stella read by read.
- **The 6532 RIOT keeps its timer count when its clock changes.**

## Not in this branch

The Atari XG-1 light gun for the 2600 (Sentinel, Shooting Arcade) builds on light-pen support that
MAME added after 0.289. It is on the development line, not here.

## Building

    git clone --branch atari-0.289 https://github.com/Miasmark/mame-Atari8bit7800
    cd mame-Atari8bit7800
    make -j$(nproc) SOURCES=src/mame/atari/a7800.cpp,src/mame/atari/atari400.cpp,src/mame/atari/a2600.cpp

Drop `SOURCES=...` for a full MAME. On Windows, use an MSYS2 MinGW64 shell, as MAME's "Compiling MAME"
documentation describes.

## Licence

MAME as a whole is under the GNU GPL v2 or later. The files this patch set changes or adds are
BSD-3-Clause, as their headers say. Nothing here was taken from GPL emulators: Stella was used only
as a black-box reference, to compare screens.
