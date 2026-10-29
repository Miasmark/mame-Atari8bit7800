#!/bin/sh
# stage.sh EXECUTABLE DIR VERSION SOURCE_URL
#
# Lays out an Atari-only MAME release in DIR: the executable, the software
# lists of the drivers in it, the licence, README-atari.md and a short
# README.txt saying where the source is (the GPL wants binaries to say so).
set -e
exe=$1; dir=$2; version=$3; source=$4
here=$(cd "$(dirname "$0")/../.." && pwd)

mkdir -p "$dir/hash"
cp "$exe" "$dir/"
for list in a2600 a2600_cass a5200 a7800 a800 a800_cass a800_flop intv intvecs xegs; do
	cp "$here/hash/$list.xml" "$dir/hash/"
done
cp "$here/COPYING" "$dir/COPYING.txt"
cp "$here/README-atari.md" "$dir/"

cat > "$dir/README.txt" <<TEXT
MAME 0.289 with the Atari patch set - $version (Atari-only build)

An unofficial build of MAME 0.289 with the Atari 2600, 7800, 8-bit and
Intellivision changes described in README-atari.md. Only these systems are
included: a2600, a2600p, a7800, a7800p, the Atari 400/800/XL/XE/XEGS and
5200 drivers and the Intellivision ones (intv, intv2, intvoice, intvecs,
...). It is not a MAMEdev release; please report problems on the fork, not
to MAME.

Source code: $source
Licence: GNU GPL v2 or later (COPYING.txt); the changed files are
BSD-3-Clause, as their headers say.

Start it from this folder so it finds the software lists in hash/, for
example:

    mame a7800 -bios skip -cart game.a78
    mame a2600 -cart game.bin
    mame a7800 -bios skip -cart2600 game.bin     (2600 game on the 7800)
    mame intv -cart game.int

BIOS images are not included. The 7800 runs without one with -bios skip;
other systems need their usual BIOS files in a roms folder.

MAME is a registered trademark of Gregory Ember.
TEXT
