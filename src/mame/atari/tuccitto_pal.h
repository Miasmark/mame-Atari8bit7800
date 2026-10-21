// license:BSD-3-Clause
// copyright-holders:Robert Tuccitto
/***************************************************************************

    Atari 7800 / 2600 palettes by Robert Tuccitto (last updated 2021-03-13)

    Measured from consoles at three temperatures: the 7800's colour phase
    shifts as it warms up, by about 25.7 degrees a hue step when cool, 26.7
    warm and 27.7 hot. These are the tables of the MiSTer and Analogue Pocket
    7800 cores (MIT), which use them for the 2600 too.

***************************************************************************/
#ifndef MAME_ATARI_TUCCITTO_PAL_H
#define MAME_ATARI_TUCCITTO_PAL_H

#pragma once

namespace atari_tuccitto {

// the "Palette" machine configuration setting
enum : u8
{
	WARM = 0,               // 26.7 degrees: the default
	COOL,                   // 25.7 degrees
	HOT,                    // 27.7 degrees
	ORIGINAL                // the palette each driver had before (MAME 0.289)
};

// [temperature][colour byte]
extern const rgb_t ntsc[3][256];
extern const rgb_t pal[3][256];

// The 7800 PAL palette's hue for each 2600 PAL hue: the two machines number
// their PAL hues differently (and 2600 hues 0, 1, E and F are grey).
extern const u8 pal_2600_hue[16];

} // namespace atari_tuccitto

#endif // MAME_ATARI_TUCCITTO_PAL_H
