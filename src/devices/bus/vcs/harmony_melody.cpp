// license:BSD-3-Clause
// copyright-holders:David Haywood, a7800 toolkit contributors
/***************************************************************************

Harmony / Melody cart for the A2600

The Harmony cart is a 'modern' A2600 cartridge, used for homebrew etc. It has
an SD slot and can be connected to a PC, roms can be transferred to it with
software on the PC side.  It uses an ARM7TDMI-S LPC2103 @ 70 Mhz to emulate
the mapper behavior of other cartridges.  It has an SD card slot for storing
game data.

The Melody version of the cartridge has been used for several recent A2600
commercial releases as well as some reproductions due to it's ability to be
programmed as any other cartridge type.  This lacks the SD slot?

There is also a 'Harmony Encore' cartridge which adds support for some of the
games the original couldn't handle due to them having larger ROMs and more
complex banking schemes (Stella's Stocking etc.)

some Harmony cart details can be found at
http://atariage.com/forums/topic/156500-latest-harmony-cart-software/


ARM based formats
-----------------

Games for these formats come with the ARM "driver" in the first 2K or 3K of
the image: it is the code that watches the 6507 bus through the LPC2103 GPIO
pins and answers it, and it is what defines the format.  It is copied to the
ARM RAM at power up and runs from there.  Running it as-is would mean
emulating the ARM in lockstep with every 6507 bus cycle, so, like other
emulators, this device implements what the driver does natively and only
runs the game's own ARM code (C code compiled for Thumb, started by the 6507
through a register) on the ARM7 core.  That code shares the ARM RAM with the
driver, so the driver state it may look at is kept where the driver keeps
it (the data fetcher tables in RAM and the music registers in the FIQ bank).

The game's ARM code runs to completion as soon as the 6507 asks for it.  On
the real cart the 6507 is fed NOPs meanwhile and then a JMP back to the
instruction after the one that started it, so the 6507 is held for the time
the ARM code takes, approximated from the cycle count of the ARM7 core.


DPC+
----

Information on the DPC+ registers is in the DPCplus.h header that comes with
batari Basic (Chris Walton, Fred Quimby, Darrell Spice Jr, CC0) and in Darrell
Spice Jr's blog entries on AtariAge:
http://atariage.com/forums/blog/148/entry-11811-dpcarm-part-6-dpc-cartridge-layout/
http://atariage.com/forums/blog/148/entry-11883-dpcarm-part-7-6507arm-exchange-of-information/
http://atariage.com/forums/blog/148/entry-11903-dpcarm-part-8-multiple-functions/

    ROM (ARM flash)
    $0000-$0BFF: driver
    $0C00-$6BFF: banks 0 to 5, 4K each, selected with $1FF6-$1FFB (bank 5
                 at power up); bank 0 usually starts with the game's ARM code
    $6C00-$7BFF: display data (copied to RAM at $40000C00)
    $7C00-$7FFF: frequency table for the music (copied to RAM at $40001C00)

    RAM
    $40000000-$40000BFF: copy of the driver, with its variables
    $40000C00-$40001BFF: display data, read and written through the fetchers
    $40001C00-$40001FFF: frequency table, partly used by the game's ARM code

    6507 registers ($1000-$107F in every bank)
    $00-$04 random number generator (32 bit LFSR), $05-$07 music amplitude
    $08-$0F data fetchers, $10-$17 windowed data fetchers,
    $18-$1F fractional data fetchers, $20-$27 window flags
    $28-$2F fractional fetcher pointer low, $30-$37 high,
    $38-$3F fractional fetcher increment
    $40-$47 window top, $48-$4F window bottom
    $50-$57 data fetcher pointer low, $68-$6F high
    $58 fast fetch mode (0 = on: the operand of LDA # below $28 is replaced by
        the register it names), $59 parameter, $5A function
    $5D-$5F waveforms, $60-$67 push, $70-$74 random number reset/write,
    $75-$77 notes, $78-$7F write

Functions: 0 resets the parameter list, 1 copies parameter 3 bytes from the
ROM address in parameters 0-1 (offset by $0C00) to where fetcher (param. 2)
points, 2 fills with the value of parameter 0, 254 and 255 run the game's ARM
code at $0C00 (in ARM state).

Two revisions of the driver are handled: the older one (Space Rocks, the 2014
Champ Games titles) clears the high nibble of a fetcher pointer when the low
byte is written while fast fetch is off, and its note registers also write to
the window top of fetchers 5-7; the newer one leaves the high nibble alone,
clears the fraction when a fractional pointer is written and uses $5C to
reset the phase of the third voice.

The music is three 32 bit phase accumulators stepped at 20 kHz (the rate of
timer 0 as set up by the driver, 70 MHz / 3501); the top five bits pick a
sample from a 32 byte waveform in the display data, and the amplitude
register gives the sum of the three samples.

***************************************************************************/


#include "emu.h"
#include "harmony_melody.h"

#include "cpu/arm7/arm7core.h"

#include "multibyte.h"

#include <algorithm>
#include <bit>

#define LOG_ARM     (1U << 1)
#define LOG_REG     (1U << 2)

//#define VERBOSE (LOG_ARM | LOG_REG)
#include "logmacro.h"

#define LOGARM(...) LOGMASKED(LOG_ARM, __VA_ARGS__)
#define LOGREG(...) LOGMASKED(LOG_REG, __VA_ARGS__)


namespace {

constexpr u32 ARM_CLOCK = 70'000'000;

// LPC2103 has 8K, LPC2148 32K
constexpr u32 ARM_RAM_MAX = 0x8000;

// budget for one call of the game's ARM code: about a frame's worth
constexpr int ARM_CALL_BUDGET = ARM_CLOCK / 50;

// DPC+ driver variables in the ARM RAM
constexpr u32 DPCP_FETCHER = 0x2a8;
constexpr u32 DPCP_WINDOW = 0x2c8;
constexpr u32 DPCP_FRACTIONAL = 0x2e8;
constexpr u32 DPCP_INCREMENT = 0x308;
constexpr u32 DPCP_WAVEFORM = 0x338;
constexpr u32 DPCP_RANDOM = 0x328;
constexpr u32 DPCP_PARAMETER_INDEX = 0x32c;
constexpr u32 DPCP_FUNCTION = 0x32d;
constexpr u32 DPCP_PARAMETERS = 0x330;
constexpr u32 DPCP_DISPLAY = 0xc00;
constexpr u32 DPCP_FREQUENCY = 0x1c00;

constexpr u32 DPCP_RANDOM_RESET = 0x2b43'5044; // "DPC+"
constexpr u32 DPCP_RANDOM_TAP = 0x10ad'ab1e;

} // anonymous namespace


//**************************************************************************
//  ARM7 core with a synchronous call
//**************************************************************************

DEFINE_DEVICE_TYPE(A26_HARMONY_ARM, a26_harmony_arm_device, "a26_harmony_arm", "Harmony/Melody ARM7TDMI")

a26_harmony_arm_device::a26_harmony_arm_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: arm7_cpu_device(mconfig, A26_HARMONY_ARM, tag, owner, clock, 4, ARCHFLAG_T, ENDIANNESS_LITTLE)
	, m_returned(false)
	, m_budget(0)
	, m_used(0)
{
}

void a26_harmony_arm_device::device_start()
{
	arm7_cpu_device::device_start();

	save_item(NAME(m_returned));
}

int a26_harmony_arm_device::call(u32 pc, u32 cpsr, u32 sp, u32 r0, int budget)
{
	set_cpsr((cpsr & ~T_MASK) | ((pc & 1) ? T_MASK : 0));
	m_r[m_reg_group[eR13]] = sp;
	m_r[m_reg_group[eR14]] = RETURN_ADDRESS;
	m_r[eR0] = r0;
	R15 = pc & ~1;

	m_returned = false;
	m_budget = budget;
	m_icount = budget;
	execute_run();

	return m_returned ? m_used : -1;
}

void a26_harmony_arm_device::return_reached()
{
	m_returned = true;
	m_used = m_budget - m_icount;
	m_icount = 0;
}

u32 a26_harmony_arm_device::fiq_reg(int reg) const
{
	return m_r[eR8_FIQ + reg - 8];
}

void a26_harmony_arm_device::set_fiq_reg(int reg, u32 data)
{
	m_r[eR8_FIQ + reg - 8] = data;
}


//**************************************************************************
//  cart device
//**************************************************************************

DEFINE_DEVICE_TYPE(A26_ROM_HARMONY, a26_rom_harmony_device, "a2600_harmony", "Atari 2600 ROM Cart HARMONY/MELODY")


a26_rom_harmony_device::a26_rom_harmony_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a26_rom_base_device(mconfig, A26_ROM_HARMONY, tag, owner, clock)
	, m_arm(*this, "arm")
	, m_maincpu(nullptr)
	, m_flash_size(0)
	, m_arm_ram_size(0)
	, m_format(FORMAT_NONE)
	, m_old_driver(false)
	, m_music_clock(20'000)
	, m_bank_offset(0)
	, m_fast_fetch(0xff)
	, m_music_ticks(0)
	, m_ff_cycle(0)
	, m_ff_offset(0)
	, m_ff_pending(false)
{
}

void a26_rom_harmony_device::device_add_mconfig(machine_config &config)
{
	// the ARM never runs on its own, only when the 6507 calls it
	A26_HARMONY_ARM(config, m_arm, ARM_CLOCK);
	m_arm->set_disable();
}

void a26_rom_harmony_device::device_start()
{
	m_arm_ram = std::make_unique<u32 []>(ARM_RAM_MAX / 4);

	save_pointer(NAME(m_arm_ram), ARM_RAM_MAX / 4);
	save_item(NAME(m_bank_offset));
	save_item(NAME(m_fast_fetch));
	save_item(NAME(m_music_ticks));
	save_item(NAME(m_ff_cycle));
	save_item(NAME(m_ff_offset));
	save_item(NAME(m_ff_pending));
}

void a26_rom_harmony_device::install_memory_handlers(address_space *space)
{
	m_maincpu = &downcast<cpu_device &>(space->device());

	space->install_read_handler(0x1000, 0x1fff, read8sm_delegate(*this, FUNC(a26_rom_harmony_device::read)));
	space->install_write_handler(0x1000, 0x1fff, write8sm_delegate(*this, FUNC(a26_rom_harmony_device::write)));

	if (m_rom_size >= 0x8000 && find_string(0, 0xc00, "DPC+"))
	{
		m_format = FORMAT_DPCP;
		m_flash_size = 0x8000;
		m_arm_ram_size = 0x2000;

		// the older driver has the same handler for $5B and $5C in its jump table
		m_old_driver = rom_r32(0x994 + 0x5b * 4) == rom_r32(0x994 + 0x5c * 4);
	}
	else
	{
		logerror("Unknown Harmony/Melody format\n");
		m_format = FORMAT_NONE;
		m_flash_size = 0x8000;
		m_arm_ram_size = 0x2000;
	}

	// the ARM sees the image as its flash, kept in 32 bit words
	m_flash = std::make_unique<u32 []>(m_flash_size / 4);
	for (u32 i = 0; i < m_flash_size; i += 4)
		m_flash[i / 4] = rom_r32(i);

	address_space &arm = m_arm->space(AS_PROGRAM);
	arm.install_rom(0, m_flash_size - 1, m_flash.get());
	arm.install_ram(0x4000'0000, 0x4000'0000 + m_arm_ram_size - 1, m_arm_ram.get());
	arm.install_read_handler(a26_harmony_arm_device::RETURN_ADDRESS, a26_harmony_arm_device::RETURN_ADDRESS + 0xf, read32sm_delegate(*this, FUNC(a26_rom_harmony_device::return_r)));
}

void a26_rom_harmony_device::device_reset()
{
	for (int reg = 8; reg <= 14; reg++)
		m_arm->set_fiq_reg(reg, 0);

	std::fill_n(&m_arm_ram[0], ARM_RAM_MAX / 4, 0);
	m_fast_fetch = 0xff;
	m_ff_pending = false;
	m_music_ticks = machine().time().as_ticks(m_music_clock);

	switch (m_format)
	{
	case FORMAT_DPCP:
		// the driver copies itself and the display data and frequencies to RAM
		for (u32 i = 0; i < 0xc00; i += 4)
			ram_w32(i, rom_r32(i));
		for (u32 i = DPCP_DISPLAY; i < 0x2000; i += 4)
			ram_w32(i, rom_r32(i + 0x6000));
		m_bank_offset = 0xc00 + 5 * 0x1000;
		break;

	default:
		m_bank_offset = 0;
		break;
	}
}


//-------------------------------------------------
//  helpers
//-------------------------------------------------

void a26_rom_harmony_device::ram_w8(u32 offset, u8 data)
{
	if (offset < m_arm_ram_size)
	{
		const int shift = (offset & 3) * 8;
		m_arm_ram[offset >> 2] = (m_arm_ram[offset >> 2] & ~(0xffU << shift)) | (u32(data) << shift);
	}
}

u8 a26_rom_harmony_device::arm_r8(u32 address) const
{
	if (address < m_flash_size)
		return m_rom[address];
	else if (address >= 0x4000'0000)
		return ram_r8(address - 0x4000'0000);
	else
		return 0;
}

u32 a26_rom_harmony_device::rom_r32(u32 offset) const
{
	return (offset + 3 < m_rom_size) ? get_u32le(&m_rom[offset]) : 0;
}

bool a26_rom_harmony_device::find_string(u32 start, u32 end, std::string_view string) const
{
	end = std::min(end, m_rom_size);
	return (start < end) && (std::search(&m_rom[start], &m_rom[end], string.begin(), string.end()) != &m_rom[end]);
}

u32 a26_rom_harmony_device::return_r(offs_t offset)
{
	if (!machine().side_effects_disabled() && !offset)
		m_arm->return_reached();
	return 0xeaff'fffe; // B .
}

// true if this read is the operand of the LDA # seen on the cycle before
bool a26_rom_harmony_device::fast_fetch_follows(offs_t offset)
{
	if (!m_ff_pending)
		return false;
	m_ff_pending = false;
	return offset == m_ff_offset && m_maincpu->total_cycles() == m_ff_cycle + 1;
}

// step the phase accumulators of the three voices up to now
void a26_rom_harmony_device::music_update()
{
	const u64 now = machine().time().as_ticks(m_music_clock);
	const u32 ticks = u32(now - m_music_ticks);
	m_music_ticks = now;
	for (int voice = 0; voice < 3; voice++)
		m_arm->set_fiq_reg(8 + voice, m_arm->fiq_reg(8 + voice) + m_arm->fiq_reg(11 + voice) * ticks);
}

void a26_rom_harmony_device::call_arm(u32 pc, u32 cpsr, u32 sp, u32 r0)
{
	const int cycles = m_arm->call(pc, cpsr, sp, r0, ARM_CALL_BUDGET);
	LOGARM("ARM call %08x: %d cycles\n", pc, cycles);
	if (cycles < 0)
	{
		logerror("ARM code at %08x did not return (PC=%08x)\n", pc, m_arm->pc());
		charge_arm_cycles(ARM_CALL_BUDGET);
	}
	else
		charge_arm_cycles(cycles);
}

// the 6507 runs NOPs while the ARM is busy, then a JMP back
void a26_rom_harmony_device::charge_arm_cycles(u32 cycles)
{
	const u64 cpu_cycles = (u64(cycles) * m_maincpu->unscaled_clock() + ARM_CLOCK - 1) / ARM_CLOCK;
	m_maincpu->adjust_icount(-int(cpu_cycles + 5));
}


//-------------------------------------------------
//  6507 side
//-------------------------------------------------

u8 a26_rom_harmony_device::read(offs_t offset)
{
	switch (m_format)
	{
	case FORMAT_DPCP:
		return dpcp_read(offset);
	default:
		return m_rom[offset & (m_rom_size - 1)];
	}
}

void a26_rom_harmony_device::write(offs_t offset, u8 data)
{
	m_ff_pending = false;

	switch (m_format)
	{
	case FORMAT_DPCP:
		dpcp_write(offset, data);
		break;
	default:
		break;
	}
}


//-------------------------------------------------
//  DPC+
//-------------------------------------------------

u8 a26_rom_harmony_device::dpcp_read(offs_t offset)
{
	if (machine().side_effects_disabled())
		return (offset < 0x80) ? 0 : m_rom[m_bank_offset + offset];

	// operand of an LDA # in fast fetch mode
	if (fast_fetch_follows(offset))
	{
		const u8 operand = m_rom[m_bank_offset + offset];
		if (operand < 0x28)
			return dpcp_register_r(operand);
	}

	if (offset < 0x80)
		return (offset < 0x28) ? dpcp_register_r(offset) : 0;

	if (offset >= 0xff6 && offset <= 0xffb)
		m_bank_offset = 0xc00 + (offset - 0xff6) * 0x1000;

	const u8 data = m_rom[m_bank_offset + offset];
	if (data == 0xa9 && !m_fast_fetch)
	{
		m_ff_pending = true;
		m_ff_offset = offset + 1;
		m_ff_cycle = m_maincpu->total_cycles();
	}
	return data;
}

void a26_rom_harmony_device::dpcp_write(offs_t offset, u8 data)
{
	if (offset >= 0x28 && offset < 0x80)
		dpcp_register_w(offset, data);
	else if (offset >= 0xff6 && offset <= 0xffb)
		m_bank_offset = 0xc00 + (offset - 0xff6) * 0x1000;
}

u8 a26_rom_harmony_device::dpcp_register_r(u8 reg)
{
	const int index = reg & 7;
	const u32 fetcher = DPCP_FETCHER + index * 4;

	switch (reg >> 3)
	{
	case 0:
		if (reg == 0)
		{
			// RANDOM0NEXT
			u32 value = std::rotr(ram_r32(DPCP_RANDOM), 11);
			if (BIT(value, 31))
				value ^= DPCP_RANDOM_TAP;
			ram_w32(DPCP_RANDOM, value);
			return value;
		}
		else if (reg == 1)
		{
			// RANDOM0PRIOR
			u32 value = ram_r32(DPCP_RANDOM);
			if (BIT(value, 31))
				value ^= DPCP_RANDOM_TAP;
			value = std::rotl(value, 11);
			ram_w32(DPCP_RANDOM, value);
			return value;
		}
		else if (reg < 5)
		{
			// RANDOM1-3
			return ram_r8(DPCP_RANDOM + reg - 1);
		}
		else
		{
			// AMPLITUDE
			return dpcp_amplitude();
		}

	case 1:
	{
		// DFxDATA
		const u32 pointer = ram_r32(fetcher);
		ram_w32(fetcher, pointer + 0x10'0000);
		return ram_r8(DPCP_DISPLAY + (pointer >> 20));
	}

	case 2:
	case 4:
	{
		// DFxDATAW, DFxFLAG: the low byte of the pointer between top and bottom
		const u32 pointer = ram_r32(fetcher);
		const u32 window = ram_r32(DPCP_WINDOW + index * 4);
		const bool inside = (window - (window << 8)) < (window - (pointer << 4));
		if (reg >= 0x20)
			return inside ? 0xff : u8(window);
		ram_w32(fetcher, pointer + 0x10'0000);
		return inside ? ram_r8(DPCP_DISPLAY + (pointer >> 20)) : u8(window);
	}

	case 3:
	{
		// DFxFRACDATA: 12.8 pointer in bits 31-12
		const u32 pointer = ram_r32(DPCP_FRACTIONAL + index * 4);
		ram_w32(DPCP_FRACTIONAL + index * 4, pointer + (ram_r32(DPCP_INCREMENT + index * 4) << 12));
		return ram_r8(DPCP_DISPLAY + (pointer >> 20));
	}
	}

	return 0;
}

void a26_rom_harmony_device::dpcp_register_w(u8 reg, u8 data)
{
	const int index = reg & 7;
	const u32 fetcher = DPCP_FETCHER + index * 4;
	const u32 fractional = DPCP_FRACTIONAL + index * 4;

	LOGREG("%s: DPC+ %02x <- %02x\n", machine().describe_context(), reg, data);

	// the older driver clears what fast fetch mode has set in the high nibble
	const u32 low_mask = m_old_driver ? ((((m_fast_fetch & 0x0f) << 8) | 0xff) << 20) : 0x0ff0'0000;

	switch (reg)
	{
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
		// DFxFRACLOW
		ram_w32(fractional, (ram_r32(fractional) & ~(low_mask | (m_old_driver ? 0 : 0x000f'f000))) | (u32(data) << 20));
		break;

	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
		// DFxFRACHI
		ram_w32(fractional, (ram_r32(fractional) & 0x0fff'ffff) | (u32(data) << 28));
		break;

	case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		// DFxFRACINC: also clears the fraction
		ram_w32(fractional, ram_r32(fractional) & ~0x000f'f000);
		ram_w8(DPCP_INCREMENT + index * 4, data);
		break;

	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		// DFxTOP
		ram_w8(DPCP_WINDOW + index * 4 + 3, data);
		break;

	case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
		// DFxBOT
		ram_w8(DPCP_WINDOW + index * 4 + 2, data);
		break;

	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		// DFxLOW
		ram_w32(fetcher, (ram_r32(fetcher) & ~low_mask) | (u32(data) << 20));
		break;

	case 0x58:
		// FASTFETCH
		m_fast_fetch = data;
		break;

	case 0x59:
	{
		// PARAMETER
		const u8 count = ram_r8(DPCP_PARAMETER_INDEX) + 1;
		ram_w8(DPCP_PARAMETER_INDEX, count);
		if (count < 11)
			ram_w8(DPCP_PARAMETER_INDEX + count, data);
		break;
	}

	case 0x5a:
		// CALLFUNCTION
		dpcp_function(data);
		break;

	case 0x5c:
		if (!m_old_driver)
		{
			// restarts the third voice
			music_update();
			m_arm->set_fiq_reg(10, 0);
			break;
		}
		[[fallthrough]];
	case 0x5b:
	case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
		// DFxHI ($5B and $5C are decoded as DF3HI and DF4HI)
		ram_w32(fetcher, (ram_r32(fetcher) & 0x0fff'ffff) | (u32(data) << 28));
		break;

	case 0x5d: case 0x5e: case 0x5f:
		// WAVEFORMx
		ram_w32(DPCP_WAVEFORM + (index - 5) * 4, 0x4000'0000 + DPCP_DISPLAY + data * 32);
		break;

	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
	{
		// DFxPUSH
		const u32 pointer = ram_r32(fetcher) - 0x10'0000;
		ram_w32(fetcher, pointer);
		ram_w8(DPCP_DISPLAY + (pointer >> 20), data);
		break;
	}

	case 0x70:
		// RRESET
		ram_w32(DPCP_RANDOM, DPCP_RANDOM_RESET);
		break;

	case 0x71: case 0x72: case 0x73: case 0x74:
		// RWRITE0-3
		ram_w8(DPCP_RANDOM + index - 1, data);
		break;

	case 0x75: case 0x76: case 0x77:
		// NOTE0-2
		music_update();
		m_arm->set_fiq_reg(11 + index - 5, ram_r32(DPCP_FREQUENCY + data * 4));
		if (m_old_driver)
			ram_w8(DPCP_WINDOW + index * 4 + 3, data);
		break;

	case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
	{
		// DFxWRITE
		const u32 pointer = ram_r32(fetcher);
		ram_w8(DPCP_DISPLAY + (pointer >> 20), data);
		ram_w32(fetcher, pointer + 0x10'0000);
		break;
	}
	}
}

void a26_rom_harmony_device::dpcp_function(u8 function)
{
	ram_w8(DPCP_FUNCTION, function);

	if (function >= 0xfe)
	{
		// the game's ARM code, entered in ARM state with the driver's
		// registers pushed on the supervisor stack
		call_arm(0x0c00, (function == 0xfe) ? 0x53 : 0xd3, 0x4000'1ffc - 9 * 4, function);
		return;
	}

	ram_w8(DPCP_PARAMETER_INDEX, 3);
	if (function == 1 || function == 2)
	{
		u32 source = ((ram_r8(DPCP_PARAMETERS) | (ram_r8(DPCP_PARAMETERS + 1) << 8)) + 0x0c00) & ~0x8000;
		u32 target = DPCP_DISPLAY + (ram_r32(DPCP_FETCHER + (ram_r8(DPCP_PARAMETERS + 2) & 7) * 4) >> 20);
		const u8 count = ram_r8(DPCP_PARAMETERS + 3);
		const int length = count ? count : 1;

		for (int i = 0; i < length; i++)
			ram_w8(target++, (function == 1) ? m_rom[(source++) & (m_rom_size - 1)] : u8(source));
		charge_arm_cycles(length * 4 + 40);
	}
}

u8 a26_rom_harmony_device::dpcp_amplitude()
{
	music_update();

	u32 sum = 0;
	for (int voice = 0; voice < 3; voice++)
		sum += arm_r8(ram_r32(DPCP_WAVEFORM + voice * 4) + (m_arm->fiq_reg(8 + voice) >> 27));
	return sum;
}
