# MAME — Atari 2600 / 7800 / 8-bit patch set (unofficial)

An unofficial patch set for MAME that improves the Atari 2600, Atari 7800 and
Atari 8-bit (400/800/XL/XE/XEGS) drivers, and the Mattel Intellivision's. It is not a MAME release and is not supported by MAMEdev:
please report problems on this fork, not to MAME. MAME is a registered trademark of Gregory Ember.

Each change is its own commit with a description of what was measured. The same work is on three
branches:

- **`master`**: current MAME (upstream's master) with all of it merged in.
- **`atari-dev`**: where the work is done, on a recent upstream MAME; it is what `master` merges.
- **`atari-0.289`**: the same changes on MAME 0.289 (tag `mame0289`): `git log mame0289..atari-0.289`.

## What cartridge is this? (2600 and 7800)

Machine Information (in the menu) and Media Control show, under the mounted file, the board it
runs on and how that was found. For example:
- 7800: "SuperGame Bankswitch + POKEY (board from the .a78 header, saves to the High Score Cart, or
  uses a SaveKey or AtariVox in port 2)", or "no header, board detected from the image";
- 2600: "HARMONY/MELODY, DPC+ driver", "F8 bankswitch, 128 bytes of RAM (Super Chip)",
  "FA2 bankswitch, saves to the Harmony's flash".

Lua scripts can read the same text as `image.media_details`.

## Colours (2600 and 7800)

Both consoles now use Robert Tuccitto's measured palettes, the ones the MiSTer and Analogue
Pocket 7800 cores use. The 7800's colours drift as the console warms up, so there are three
(Machine Configuration, **Palette**): Warm (default, 26.7 degrees a hue step), Cool (25.7), Hot
(27.7), or Original for the palettes MAME had before. One setting covers the 7800 and its 2600
mode. With Warm, every colour matches the Pocket core's own screenshots of a colour test cartridge.

## Atari 7800 (`a7800`, `a7800p`)

- **One cartridge slot for everything.** The Cartridge slot (File Manager, `-cart`) takes `.a78`,
  `.bin` and `.a26` files:
  - a 2600 game is passed to the 2600 slot and runs in 2600 mode (`-cart2600` still works too);
  - a 7800 image without its header is recognised: known dumps from the software list (with their
    exact board), others by the 7800 BIOS's own cartridge check, mapped flat up to 48K or as a 128K
    or 144K SuperGame board;
  - the Software menu also lists the 2600 software list (`a2600:name` on the command line).
- **2600 mode.** A 2600 cartridge runs as on a real 7800. The BIOS finds it,
  runs its 2600 bootstrap and locks INPTCTRL with MARIA off. From then on:
  - the TIA draws the screen;
  - the CPU and the RIOT run at 1.19 MHz;
  - the CPU sees a 6507's 13-bit bus.

  All of MAME's 2600 cartridge types work, including the new ones below, and Harmony/ARM carts too.
  Without a BIOS (or with `-bios skip`) a 2600 cartridge starts straight in 2600 mode. Results:
  - **Compared with the `a2600` driver** on 251 USA games: 242 are pixel-identical at 10 s. The other
    9 keep identical RAM; `a2600` uses a 3.579575 MHz clock, the 7800 its own 3.579545 MHz crystal.
  - **Through 7800OpenBIOS:** all 16 games tested boot into their attract modes.
  - **Soft reset and save states** work.
  - **Known limit:** Harmony ARM games (DPC+, CDF) stay on a black screen when started through a
    BIOS: its 7800-mode cartridge check reads `$FFF8`, which switches their bank. Use `-bios skip`.
- **BankSet cartridges**, all 11 forms (CPU half and MARIA half, BankSet RAM, 52K halves, POKEY at
  `$4000` or `$0800`), plus the spec's rules: display lists must be in console RAM, and no code may
  run from cart RAM. Also header `$2006` (and `$a006` with POKEY at `$0800`): a SuperGame bankset with
  one 16K RAM at `$4000` that the CPU and MARIA share, as on the MiSTer and Pocket cores; display
  lists and code may be in that RAM. All 10 programs of the Bankset Test Suite v1 (`.a78`) pass.
- **POKEY at `$0800`**, and **dual POKEY** at `$0440`+`$0450` (both chips at half gain).
- **RAM at `$4000` without SuperGame** (header `$0004`, with or without POKEY at `$0450` or `$0800`) on
  an image under 64K is a flat board with the RAM over `$4000`-`$7fff`, as on the Pocket core; MAME
  used to make it SuperGame, where a write to `$8000`-`$bfff` would page in the wrong bank (Star
  Raiders, Mike Saarna's port).
- **Activision `.a78` images in the usual block order** now boot.
- **MARIA timing from the 7800-devtools a7800 fork**, with MARIA's DMA in its own address space, so
  debugger and Lua watches can tell its reads from the CPU's; updated to the 7800 Software Guide's
  revised DMA timing (October 2026): a display list interrupt costs no DMA time, holey character
  objects cost 6 or 9 cycles, and the first DMA of each frame 8 more.
- **MARIA's DMA cost checked on a real NTSC 7800** with the a7800 toolkit's on-screen measuring
  carts (15 display shapes). The first zone of each frame was drawn one line too tall, which
  cost a line of DMA a frame and put the picture a line low; a line past the DMA limit now holds
  the CPU for 107 cycles. Every shape now costs what it does on the console to within 21 cycles
  a frame, 12 of the 15 within 7.
- **No BIOS file needed.** Without one the 7800 boots straight into the cartridge, as with
  `-bios skip`, and the system is not shown as missing its ROMs; MAME notes at start that the
  optional BIOS file is missing. `bios skip` in `a7800.ini` (or `a7800p.ini`) makes skipping the
  default and drops that note. With the BIOS file in the ROM folder it runs as before; `-bios skip`
  still skips it. Skipping starts the cartridge where 7800OpenBIOS does in the frame, without the
  signature check, so a signed and an unsigned image of the same game start on the same cycle and
  an input recording plays back the same on both.
- **High Score Cartridge from its own image:** `-cart "High Score Cartridge.a78" -cart2 game.a78`
  (the software list entry `hiscore` works as before). Its scores are saved in the nvram folder.
  Any board runs through it, bankset ones included: its RAM ($1000-$17FF) and ROM are the CPU's
  only, and MARIA sees the game. The same goes for the XM and XBoarD (software list entries `xm`
  and `xboard`, the game as `-cart2`): a game's own POKEY at $0450 or $0800 answers through them.
- **SaveKey / AtariVox in controller port 2** (Machine Configuration, Controller Port 2): Auto plugs
  one in when the cartridge's header asks for it; Joystick or SaveKey / AtariVox force the choice.
  It also works in 2600 mode. The AtariVox's voice is there too (see the 2600's AtariVox below).
  Like all MAME nvram, the memory's file is kept per BIOS choice (`nvram/a7800/` with the default
  BIOS, `nvram/a7800_2/` without a BIOS).

## Atari 2600 (`a2600`, `a2600p`)

- **XG-1 light gun** (`-joyport1 xg1`): trigger on pin 1, light sensor on pin 6, seeing what the
  TIA has just drawn where it is aimed. Sentinel and Shooting Arcade could not be tested.
- **SaveKey** for a controller port: `-joyport2 savekey` (or Slot Devices in the menu). The 32K
  EEPROM is kept in the nvram folder, shared by all games as a real one is.
- **AtariVox**, the SaveKey's memory and a voice: `-joyport2 atarivox`, with a speaker of its own.
  The SpeakJet's sound tables were never published, so the voice is the openfpga-7800 (Analogue
  Pocket) core's speech engine, run clock for clock with the core's program and tables: it says what
  the Pocket says (checked sample for sample against the core's reference over its test cartridge's
  eight phrases). Its serial control mode follows the SpeakJet's manual (`\0` enters it; `X`, or a
  backslash and a letter, leaves it), where the core enters it on any backslash; Star Raiders, whose
  phrases begin with `\V0 `, speaks here and not on the Pocket.
- **FA2 (Star Castle Arcade) saves** its Hall of Fame to the Harmony's flash, kept in the nvram
  folder.
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

## Mattel Intellivision (`intv`, `intv2`, `intvoice`, `intvecs`, ...)

Corrections found while writing a clean-room Intellivision core for the Analogue Pocket, from GI's
CP-1600/CP1610 manuals, the Intellivision wiki and recordings of a real console:

- **The STIC takes the bus from the CPU.** MAME took its stolen cycles off the CPU from a timer,
  where that did nothing: the CPU had the whole frame. Now the STIC asserts the CP1610's BUSRQ, the
  CPU stops at the end of its next interruptible instruction until it goes (and two cycles more),
  and with the display on it takes about 1,460 cycles of a frame's 14,934 (1,350 with a vertical
  delay). A row of cards is copied when the CPU stops for it; bus copy starts when the CPU takes the
  VBlank interrupt. Games that time themselves against the display (Sewer Sam, TRON Solar Sailer)
  drifted against it.
- **A frame is 14,934 CPU cycles** (262 lines of 57), not 14,934.68.
- **Interrupts are taken after the first interruptible instruction that ends after the request**;
  they were one instruction late.
- **CP1610 cycles:** SETC takes 4 cycles (it was 6), SARC Rr,2 takes 8 (6). HLT halts the CPU
  after the next interruptible instruction; it did nothing.
- **ECS:** all 16 ROM pages, so IntyBASIC's ECS start-up can page the ECS ROMs out; its second sound
  chip is an AY-3-8917 (no second envelope/volume control bit).
- **Intellivoice:** the SP0256 speaks at 10 kHz (it was 9,285 Hz, 7.7 % too low and too slow);
  upstream MAME has this too.

The exact number of stolen cycles is still to be measured on a console.

## Core and debugging

- **`-debugger script`**: a windowless debugger whose stops hold while Lua runs, so scripts can use
  breakpoints and stepping.
- **`-playback_offset N` / `-playback_force`**: play an `.inp` file on a game that starts on another
  frame.
- **The 6532 RIOT timer is cycle-exact.** It ran one or two cycles early, which broke games that
  poll INTIM for 0 once a line (Stay Frosty 2). A read after the timer runs out now resumes counting
  on the prescaler's own phase. Checked against Stella read by read.
- **The 6532 RIOT keeps its timer count when its clock changes.**

## Downloads

Atari-only builds (a2600, a7800, the Atari 8-bit computers and the 5200, and the Intellivision) are made by
`.github/workflows/atari-release.yml`: every push to `master` or `atari-0.289` keeps them as workflow
artifacts, and a tag named `atari-*` puts them in a draft release on the fork's Releases page:

- **Windows x64**: unzip and run `mame.exe` from that folder. Windows may warn that it is
  unsigned: More info, Run anyway.
- **macOS, Apple silicon (macOS 11 or later)**: unzip, then either right-click `mame` and choose
  Open the first time, or run `xattr -dr com.apple.quarantine` on the folder. The build is not
  signed by a developer.
- **Linux x86-64**: one AppImage carrying its libraries (glibc 2.34 or later). `chmod +x` it and
  run it; without FUSE, add `--appimage-extract-and-run`.

BIOS images are not included; the 7800 does not need one (it starts straight in the cartridge). The
Intellivision needs its EXEC and GROM (`exec.bin`, `ro-3-9503-003.u21`).

## Building

    git clone https://github.com/Miasmark/mame-Atari8bit7800
    cd mame-Atari8bit7800
    make -j$(nproc) SOURCES=src/mame/atari/a7800.cpp,src/mame/atari/atari400.cpp,src/mame/atari/a2600.cpp,src/mame/mattel/intv.cpp

Drop `SOURCES=...` for a full MAME. On Windows, use an MSYS2 MinGW64 shell, as MAME's "Compiling MAME"
documentation describes.

## Licence

MAME as a whole is under the GNU GPL v2 or later. The files this patch set changes or adds are
BSD-3-Clause, as their headers say. Nothing here was taken from GPL emulators: Stella was used only
as a black-box reference, to compare screens.
