// license:BSD-3-Clause
// copyright-holders:David Haywood, a7800 toolkit contributors
#ifndef MAME_BUS_VCS_HARMONY_MELODY_H
#define MAME_BUS_VCS_HARMONY_MELODY_H

#pragma once

#include "rom.h"
#include "cpu/arm7/arm7.h"


// ======================> a26_harmony_arm_device

// ARM7TDMI of the Harmony/Melody, used to run the game's own ARM code
// synchronously when the 6507 asks for it
class a26_harmony_arm_device : public arm7_cpu_device
{
public:
	static constexpr u32 RETURN_ADDRESS = 0x7fff'fff0;

	a26_harmony_arm_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	// runs the routine at pc until it returns to RETURN_ADDRESS; gives the
	// number of cycles it took, or -1 if it did not return within the budget
	int call(u32 pc, u32 cpsr, u32 sp, u32 r0, int budget);
	void return_reached();

	// the music state of the Harmony drivers lives in the FIQ bank
	u32 fiq_reg(int reg) const;
	void set_fiq_reg(int reg, u32 data);

protected:
	virtual void device_start() override ATTR_COLD;

private:
	bool m_returned;
	int m_budget;
	int m_used;
};


// ======================> a26_rom_harmony_device

class a26_rom_harmony_device : public a26_rom_base_device
{
public:
	a26_rom_harmony_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	virtual void install_memory_handlers(address_space *space) override;

protected:
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
	enum : u8
	{
		FORMAT_NONE = 0,
		FORMAT_DPCP,
		FORMAT_CDF,     // CDF version 0/1
		FORMAT_CDFJ,
		FORMAT_CDFJP    // CDFJ+
	};

	u8 read(offs_t offset);
	void write(offs_t offset, u8 data);
	u32 return_r(offs_t offset);
	u64 timer1_ticks() const;
	u32 timer1_r(offs_t offset);
	void timer1_w(offs_t offset, u32 data);

	// helpers for the ARM side memory
	u8 ram_r8(u32 offset) const { return offset < m_arm_ram_size ? u8(m_arm_ram[offset >> 2] >> ((offset & 3) * 8)) : 0; }
	void ram_w8(u32 offset, u8 data);
	u32 ram_r32(u32 offset) const { return offset < m_arm_ram_size ? m_arm_ram[offset >> 2] : 0; }
	void ram_w32(u32 offset, u32 data) { if (offset < m_arm_ram_size) m_arm_ram[offset >> 2] = data; }
	u8 arm_r8(u32 address) const;
	bool find_string(u32 start, u32 end, std::string_view string) const;
	u32 rom_r32(u32 offset) const;

	u32 driver_word(u32 mask, u32 match, u32 start = 0) const;
	bool fast_fetch_follows(offs_t offset);
	void music_update();
	void call_arm(u32 pc, u32 cpsr, u32 sp, u32 r0);
	void charge_arm_cycles(u32 cycles);

	// DPC+
	u8 dpcp_read(offs_t offset);
	void dpcp_write(offs_t offset, u8 data);
	u8 dpcp_register_r(u8 reg);
	void dpcp_register_w(u8 reg, u8 data);
	void dpcp_function(u8 function);
	u8 dpcp_amplitude();

	// CDF, CDFJ, CDFJ+
	void cdf_setup();
	u8 cdf_read(offs_t offset);
	void cdf_write(offs_t offset, u8 data);
	void cdf_bank(offs_t offset);
	u8 cdf_stream_r(u8 stream, bool jump);
	u8 cdf_amplitude();
	u32 cdf_pointer(u8 stream) const { return ram_r32(m_cdf_streams + stream * 4); }

	required_device<a26_harmony_arm_device> m_arm;
	cpu_device *m_maincpu;

	std::unique_ptr<u32 []> m_flash;
	std::unique_ptr<u32 []> m_arm_ram;
	u32 m_flash_size;
	u32 m_arm_ram_size;
	u8 m_format;
	bool m_old_driver;

	// CDF family driver parameters
	u32 m_cdf_streams;      // RAM offset of the data stream pointers
	u8 m_cdf_amplitude;     // LDA # operand of the amplitude, after the last stream
	u8 m_cdf_jumps;         // number of fast jump streams
	u8 m_cdf_shift;         // 12.20 or 16.16 stream pointers
	u32 m_cdf_waveforms;    // RAM offset of the waveform pointers
	s32 m_cdf_latch;        // RAM offset where the amplitude waits for the next read, or -1
	bool m_cdf_digital;     // sampled audio mode available
	u8 m_cdf_digital_shift;
	u8 m_cdf_ff_opcodes[3]; // opcodes for fast fetch
	u8 m_cdf_ff_offset;     // fast fetch operand offset
	s32 m_cdf_banks[8];     // ROM offsets for $1FF4-$1FFB
	u32 m_cdf_entry;
	u32 m_cdf_stack;
	u8 m_mode;

	u32 m_timer1_control;
	u32 m_timer1_prescale;
	u32 m_timer1_count;
	u64 m_timer1_base;

	u32 m_bank_offset;
	u8 m_fast_fetch;
	u64 m_music_ticks;

	// operand of an LDA # (or JMP) that fast fetch takes over
	u64 m_ff_cycle;
	offs_t m_ff_offset;
	bool m_ff_pending;
	u8 m_ff_stream;
	u8 m_ff_remaining;
};


// device type definition
DECLARE_DEVICE_TYPE(A26_ROM_HARMONY, a26_rom_harmony_device)

#endif // MAME_BUS_VCS_HARMONY_MELODY_H
