// license:BSD-3-Clause
// copyright-holders:Miasmark
/**********************************************************************

    AtariVox: a SaveKey's memory and a speech chip, on a controller port

**********************************************************************/

#ifndef MAME_BUS_VCS_CTRL_ATARIVOX_H
#define MAME_BUS_VCS_CTRL_ATARIVOX_H

#pragma once

#include "savekey.h"


// ======================> atarivox_voice_device

// The voice is an approximation of the SpeakJet's, which it says on screen once a game first talks
// to it. It carries no imperfect-sound flag: the 7800 has one in every machine, for its port 2,
// and the flag would mark every 7800 game. The AtariVox on a controller port, below, has it.
class atarivox_voice_device : public device_t, public device_sound_interface
{
public:
	// the openfpga-7800 core's clk_sys (NTSC), which its serial timing and 16,384 Hz tick come from
	static constexpr u32 CLOCK = 14'318'182;

	atarivox_voice_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	void line_w(int state);     // pin 1 (Up) as the console drives it: 1 when let go
	int ready_r();              // pin 2 (Down): low while the buffer is half full

protected:
	virtual void device_start() override ATTR_COLD;
	virtual void sound_stream_update(sound_stream &stream) override;

private:
	TIMER_CALLBACK_MEMBER(rx_sample);
	void received(u8 data);
	void step();

	sound_stream *m_stream;
	emu_timer *m_rx_timer;

	// the serial input and the 64-byte buffer
	u8 m_line;
	u8 m_rx_bit;                // 0 idle, 1 start, 2-9 data, 10 stop
	u8 m_rx_shift;
	bool m_scp;                 // in serial control mode
	bool m_escape;              // a backslash: the next byte says which mode
	u8 m_fifo[64];
	u8 m_fifo_rp;
	u8 m_fifo_count;

	// the speech engine
	bool m_running;             // from the first start bit
	u32 m_mem[256];
	u16 m_pc;
	u64 m_clk;
	u8 m_flags;
	u16 m_lfsr;
	s16 m_sample;
};


// ======================> vcs_atarivox_device

class vcs_atarivox_device : public vcs_savekey_device
{
public:
	static constexpr feature_type imperfect_features() { return feature::SOUND; }

	vcs_atarivox_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	virtual uint8_t vcs_joy_r() override;
	virtual void vcs_joy_w(uint8_t data) override;

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
	required_device<atarivox_voice_device> m_voice;
};


// device type definitions
DECLARE_DEVICE_TYPE(ATARIVOX_VOICE, atarivox_voice_device)
DECLARE_DEVICE_TYPE(VCS_ATARIVOX, vcs_atarivox_device)

#endif // MAME_BUS_VCS_CTRL_ATARIVOX_H
