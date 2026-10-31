# Mattel Intellivision in this fork, measured on a real console

The fork's Intellivision changes ([README-atari.md](README-atari.md), "Mattel Intellivision") came
from GI's CP-1600/CP1610 manuals, the Intellivision wiki and recordings. On 2026-10-31 five small
test cartridges ran on a real console: an original Mattel Model 2609 ("Intelligent Television",
hardwired controllers) with the ECS, through a PiRTO II flash cartridge, on a CRT. They were
written for the clean-room Intellivision core for the Analogue Pocket (its `docs/HW_TESTS.md`).
Each one settles a question the documents leave open and shows its result as text.

This page puts the console's screens beside this fork's: build `095ec275`, whose Intellivision
code is unchanged up to `4eada4de`. The same cartridge images were run headless with the console's
EXEC and GROM. Where the fork differed, the code involved is named (file:line in that build) with
the behaviour the console showed. The fork was then corrected to the console (the commits after
`4eada4de` on 2026-10-31); the last column is the same tests on it.

| Test | Console vs. build `095ec275` | Now |
|---|---|---|
| hw_cpuodd: undefined CPU cases | **differs** on 6 of 7 cases (NEGR of 0, SDBD with R6, SDBD with direct mode, SDBD before MVO, J with II = 3, MVO@ with the pointer as source) | all 11 words the console's |
| hw_steal: cycles the STIC takes a frame | **close**: the fork takes 1,412 cycles a frame, the console 1,365 | 1,373 |
| hw_rowcopy: when a row of cards is copied | **close**: one column (15 cycles) late on 9 of the 24 steps | all 24 steps the console's |
| hw_intlate: where the VBlank interrupt is taken | **differs**: 48 cycles, plus the sampling point | `1C 1C 1B 1B 1B 1B`: K1 one later (the sampling point) |
| hw_ecskeys: the ECS keyboard matrix | **differs**: no masking, and no column-to-row scan (from the code) | the console's, for K, K + 8 and K + 8 + F |

What changed, besides the CPU cases: the CP1610 lends the bus at the end of any instruction, not
only an interruptible one (what `hw_rowcopy`'s copy loop of MVOs needs); the STIC fetches 12 rows,
with no 13th when the vertical delay is 0 (its request ran into the next frame and held back the
VBlank interrupt: most of `hw_intlate`'s 48 cycles); the request at the end of VBlank is held about
100 cycles, not 42. These three were fitted to `hw_steal`, `hw_intlate` and `hw_rowcopy` together,
not measured one by one. The ECS keyboard is modelled without diodes, both ways.

## 1. Undefined CPU cases (hw_cpuodd)

Run once at start-up with interrupts off. The variables are at `$0300`. The sources disagree on
all of these: GI's manuals call them "not supported", MAME and the wiki chose differently.

| Case | Console | This fork | Where | What the console does |
|---|---|---|---|---|
| NEGR of 0: the status word after it (GSWD) | `5050` (C = 1) | `4040` (C = 0) | `cp1610.cpp:197` `cp1610_negr`: the 16-bit `temp` loses the carry out of `(r ^ $FFFF) + 1` | C is set when the operand is 0 (as the wiki says) |
| SWAP R0, 2 of `$1234` | `3434` | `3434` | | as the fork |
| SDBD + MVI@ R6 over `$11AA`, `$22BB`: the word, R6 moved | `BBBB`, `0001` | `AABB`, `0002` | `cp1610.cpp:1544` `cp1610_sdbd_mviat_d`: two pre-decrements | **one** decrement; the same word read twice, so both halves are its low byte (`$BBBB`); R6 down by 1. The wiki gives 10 cycles; not measured |
| SDBD + MVO `$ABCD` to M1: M1, M1 + 1 | `ABCD`, `0000` | `0000`, `0000` | `cp1610.cpp:2004-2011` `cp1610_do_sdbd`: `0x240-0x278` are illegal, so nothing is written and the address word then runs as an instruction | SDBD is **ignored**: one ordinary 16-bit write |
| SDBD + MVI M3, R0 (direct; M3 = `$1357`) | `0000` | `00A1` | `cp1610.cpp:2013` (and the other direct modes after SDBD): decoded as `MVI@ R0` | the **address is double-byte data**: the low bytes of the next two words, low first, form the address, then one 16-bit read there. The instruction is three words long. In the test the address formed was a location holding 0 |
| J with interrupt bits 11: `J`/`N`, then the ISRs taken after it with interrupts off before | `004A` ('J', taken), `0005` | `004E` ('N'), `0000` | `cp1610.cpp:2104` `cp1610_do_jumps` `case 0x303`: illegal, no jump | it **jumps and enables interrupts**, as JE (`cp1610.cpp:1973`) |
| MVO@ R4, R4 at M2 = `$0312`: the word written, R4 after | `0313`, `0313` | `0312`, `0313` | `cp1610.cpp:1036` `cp1610_mvoat_i`: writes `m_r[s]` before `m_r[m]++` | the **incremented** pointer is written. Measured for R4; R5 and R6 (the same auto-increment path) presumably the same |

The JSR forms with interrupt bits 11 (`case 0x003`, `0x103`, `0x203`) were not tested.

## 2. Cycles the STIC takes (hw_steal)

Phases of 128 frames alternate, display off and display on. The main loop counts its passes
(52 cycles each) over frames 8-127 of each phase. Stolen cycles a frame = (OFF - ON) x 52 / 120.

| | OFF | ON | OFF - ON | Stolen a frame |
|---|---|---|---|---|
| Console | 33,431 | 30,280 | 3,151 | **1,365** |
| This fork | 33,438 | 30,180 | 3,258 | 1,412 |
| MAME 0.289 | | | about 0 | none (fixed by this fork) |

- **With the display on**, the fork takes 47 cycles a frame too many. With 13 bus requests a
  frame (the frame request and 12 rows), that is about 3.6 cycles a request. The candidates are
  `ROW_BUSRQ` (110), `FRAME_BUSRQ` (42, "approximate"), and the CPU's 2-cycle exit after BUSRQ
  goes (`stic.h:49-50`, `intv_m.cpp:671-690`).
- **With the display off**, the console's loop gets 7 passes fewer over 120 frames: about 3
  cycles a frame less for the program. Either the console's frame is about 3 cycles shorter
  than 14,934, or its interrupt entry costs about 3 cycles more. This test cannot tell which.

## 3. When a row is copied (hw_rowcopy)

Every frame an ISR of constant length fills rows 0-10 with an old pattern (white `-`). A writer
then waits 15 x (148 + STEP) cycles and writes a new pattern (red `#`) over rows 0-10, one word
every 15 cycles. The STIC copies a row in one go when the CPU stops for it, so the row being
written at that moment shows `#` up to the writer's column. STEP goes 0-23, 90 frames each. Row 3
is the mixed row on the console and on the fork. The table counts the `#` in row 3 (columns 1-19):

| STEP | 0-5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Console | 19 (at 3) | **18** | | **16** | | | | | | | | **8** | | **6** | | **4** | | | **1** |
| This fork | 19 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 |

- The console was photographed at steps 3, 6, 8, 16, 18, 20 and 23 (in bold). Its counts fit
  min(19, 24 - STEP), one straight line.
- The fork is one column (15 cycles) late from step 6 to step 14, and then jumps two columns
  between 14 and 15. It agrees with the console from step 15 on.
- So row 3's copy moves with the writer's phase on the console, but the fork's request is
  granted up to 15 cycles later at some phases.

## 4. Where the VBlank interrupt is taken (hw_intlate)

A lock frame (display on) and a probe frame (display off) alternate. The lock frame's row
fetches fix the CPU's phase. In the probe frame, the ISR returns into probe k (k = 0-5: 24 + k
cycles of padding, a long fixed delay, then a train of 60 `INCR R0`, 6 cycles each). The next
interrupt cuts the train, and the screen shows how far it got (hex):

| | k0 | k1 | k2 | k3 | k4 | k5 |
|---|---|---|---|---|---|---|
| Console | 1C | 1B | 1B | 1B | 1B | 1B |
| This fork | 14 | 14 | 13 | 13 | 13 | 13 |

- On the fork, the interrupt cuts the train 8 instructions (48 cycles) earlier than on the
  console.
- The step from one value to the next falls one padding cycle later (between k1 and k2, not k0
  and k1), so the fork samples the request at a point 1 cycle different from the console's.
- The test measures, as one sum, the time from the lock frame's last row fetch to the next VBlank
  interrupt and the point where the request is sampled. So the 48 cycles may lie in the fork's
  row-fetch and VBlank timing (`VBLANK_END` 3,790, `FIRST_FETCH` 3,933, `ROW_BUSRQ`: `stic.h:49-52`)
  as much as in the CPU's interrupt logic; this test alone cannot say which.
- The Pocket core reads `1D 1C 1C 1C 1C 1C` here: one later than the console.

## 5. The ECS keyboard (hw_ecskeys)

The test scans the keyboard both ways through the ECS's second PSG:

- **A>B**: port A drives one row low, port B reads the columns;
- **B>A**: port B drives one column low, port A reads the rows.

| Keys held | Console A>B | Console B>A |
|---|---|---|
| K | row 1: column 2 | column 2: row 1 |
| K + 8 (one row) | row 1: columns 2 and 5 | nothing |
| K + 8 + F (three corners of a rectangle) | nothing | nothing |

The computer keyboard has no diodes. A pressed key joins its row and column. The lines form
connected groups. A group reads 1 if any of its lines is driven 1, else 0 if one is driven 0,
else 1 (the pull-ups). The ECS's own scan drives one line low and the other seven high, so keys
joined to a line driven high vanish (masking), in both directions. Diodes are only in the music synthesizer.

This fork (code reading; the run needs ECS keys held):

- `ecs_ctrl.cpp:276` `intvecs_keybd_device::read_portB` ANDs the selected rows: every key is seen
  at its place, nothing is masked. The fork would show K + 8 + F as three keys.
- The keyboard has no `read_portA` (`ecs_ctrl.h:32` returns `$FF`), so the B>A scan sees nothing,
  even for one key.

## 6. From the documents, not yet measured

- **The ECS UART** (`$00E0-$00E3`, the cassette and printer port): unmapped in build `095ec275`
  (reads 0). The ECS ROM's tape save polls its "transmitter ready" bit, so a BASIC or World Series
  Major League Baseball tape save waits forever at "Press RETURN". A status of `$0002` (ready, data
  discarded) lets a save finish, as a real ECS with no tape does. The fork now reads `$0002` there.

## Notes

- **Running the tests on the fork.** The cartridges are BIN + CFG at `$5000`. Load each `.bin`
  with `-cart`, and read the results from `$0300` (Lua, `spaces["program"]:read_u16`) or off the
  screen.
- **Planned follow-ups.** Two more tests are being prepared to separate the remaining unknowns:
  the frame length against the interrupt cost, and the per-request cost. This page will be
  updated with their results.
- **No ROM data here.** No ROM data or console captures are in this repository; the console's
  results are the numbers above.
