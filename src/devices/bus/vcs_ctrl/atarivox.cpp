// license:BSD-3-Clause
// copyright-holders:Miasmark
/**********************************************************************

    AtariVox

    A SaveKey's 32K EEPROM on pins 3 and 4 of a controller port (usually
    the right one), and a SpeakJet speech chip on pins 1 and 2: a game
    sends it codes over pin 1 (Up), 8N1 at 19,200 baud, driving the pin
    low by making it an output and letting it go by making it an input,
    and reads pin 2 (Down), low while the chip's 64-byte buffer is half
    full, before each byte.

    The SpeakJet's own sound tables were never published, so the voice is
    the openfpga-7800 core's (docs/ATARIVOX.md there): a small processor
    whose program (atarivox.ipp) both reads the codes and synthesizes
    the sound, at 16,384 samples a second. This is that processor clock for
    clock, as the core's tools/atarivox/voxeng.py is, so it says what the
    core says. The serial input is the core's too, a bit every 744 clocks,
    with serial control mode as the SpeakJet's manual has it: a backslash
    and 0 enter it, and then R clears the buffer and stops the voice, V
    says "Ready", and X, or a backslash and a letter, leaves it; the rest
    is swallowed. (The core enters it on any backslash: Star Raiders begins
    its phrases with "\V0 " to be sure it is out of the mode, then speaks.)

    The engine starts with the first start bit it sees, as if the AtariVox
    were switched on then, so games that never speak cost nothing; MAME
    then says, once, that the voice is an approximation. The
    console's reset doesn't reach the AtariVox, so the device has no reset.

**********************************************************************/

#include "emu.h"
#include "atarivox.h"

#include "speaker.h"

#include "atarivox.ipp"


DEFINE_DEVICE_TYPE(ATARIVOX_VOICE, atarivox_voice_device, "atarivox_voice", "AtariVox voice")
DEFINE_DEVICE_TYPE(VCS_ATARIVOX, vcs_atarivox_device, "vcs_atarivox", "AtariVox")


namespace {

constexpr unsigned TICK_DIV = 874;      // 16,384 Hz from 14.32 MHz
constexpr unsigned BIT_CLKS = 744;      // 19,244 baud
constexpr unsigned CLK_PER_INSTR = 4;

enum : unsigned
{
	OP_ADD, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_ADDI, OP_ANDI, OP_ORI, OP_LUI, OP_MUL,
	OP_HI12, OP_SIN, OP_LDR, OP_LDRS, OP_LDM, OP_STM, OP_BEQ, OP_BNE, OP_BLT, OP_BGE,
	OP_BLTU, OP_JAL, OP_JR, OP_IN, OP_OUT
};

enum : u8
{
	FLAG_TICK = 0x01,
	FLAG_ABORT = 0x04,
	FLAG_READY = 0x08
};

// does the tick divider wrap on any clock from c0 to c1? It wraps on clocks TICK_DIV - 1, 2 * TICK_DIV - 1, ...
bool wraps(u64 c0, u64 c1)
{
	return c1 >= c0 && (c1 + 1) / TICK_DIV > c0 / TICK_DIV;
}

u32 drom(unsigned addr)
{
	return (addr < std::size(AVOX_DROM)) ? AVOX_DROM[addr] : 0;
}

} // anonymous namespace


//**************************************************************************
//  the voice
//**************************************************************************

atarivox_voice_device::atarivox_voice_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, ATARIVOX_VOICE, tag, owner, clock),
	device_sound_interface(mconfig, *this),
	m_stream(nullptr),
	m_rx_timer(nullptr),
	m_line(1),
	m_rx_bit(0),
	m_rx_shift(0),
	m_scp(false),
	m_escape(false),
	m_fifo_rp(0),
	m_fifo_count(0),
	m_running(false),
	m_pc(0),
	m_clk(0),
	m_flags(0),
	m_lfsr(0xace1),
	m_sample(0)
{
	std::fill(std::begin(m_fifo), std::end(m_fifo), 0);
	std::fill(std::begin(m_mem), std::end(m_mem), 0);
}

void atarivox_voice_device::device_start()
{
	m_stream = stream_alloc(0, 1, clock() / TICK_DIV);
	m_rx_timer = timer_alloc(FUNC(atarivox_voice_device::rx_sample), this);

	save_item(NAME(m_line));
	save_item(NAME(m_rx_bit));
	save_item(NAME(m_rx_shift));
	save_item(NAME(m_scp));
	save_item(NAME(m_escape));
	save_item(NAME(m_fifo));
	save_item(NAME(m_fifo_rp));
	save_item(NAME(m_fifo_count));
	save_item(NAME(m_running));
	save_item(NAME(m_mem));
	save_item(NAME(m_pc));
	save_item(NAME(m_clk));
	save_item(NAME(m_flags));
	save_item(NAME(m_lfsr));
	save_item(NAME(m_sample));
}


void atarivox_voice_device::line_w(int state)
{
	state = state ? 1 : 0;
	if (state == m_line)
		return;
	m_line = state;

	// a falling edge while idle is a start bit: check it half a bit in
	if (!state && !m_rx_bit)
	{
		if (!m_running)
		{
			m_stream->update();
			m_running = true;
			machine().popmessage("AtariVox: the voice is an approximation of the SpeakJet's");
		}
		m_rx_bit = 1;
		m_rx_timer->adjust(attotime::from_ticks(BIT_CLKS / 2, clock()));
	}
}

int atarivox_voice_device::ready_r()
{
	m_stream->update();
	return (m_fifo_count < std::size(m_fifo) / 2) ? 1 : 0;
}

TIMER_CALLBACK_MEMBER(atarivox_voice_device::rx_sample)
{
	switch (m_rx_bit)
	{
	case 1:     // the start bit: still low, or it was a glitch
		m_rx_bit = m_line ? 0 : 2;
		break;
	case 10:    // the stop bit: a byte if it is high
		m_rx_bit = 0;
		if (m_line)
			received(m_rx_shift);
		break;
	default:    // data, least significant bit first
		m_rx_shift = (m_rx_shift >> 1) | (m_line << 7);
		m_rx_bit++;
		break;
	}
	if (m_rx_bit)
		m_rx_timer->adjust(attotime::from_ticks(BIT_CLKS, clock()));
}

void atarivox_voice_device::received(u8 data)
{
	m_stream->update();
	if (m_escape)
	{
		// the control value: node 0, which every SpeakJet answers, enters serial control mode; another
		// node or a letter leaves it, and what follows is spoken again
		m_escape = false;
		m_scp = data == '0';
	}
	else if (data == '\\')
	{
		m_escape = true;
	}
	else if (m_scp)
	{
		switch (data)
		{
		case 'X':
			m_scp = false;
			break;
		case 'R':   // clear the buffer and stop
			m_fifo_rp = m_fifo_count = 0;
			m_flags |= FLAG_ABORT;
			break;
		case 'V':   // say "Ready"
			m_flags |= FLAG_READY;
			break;
		}
	}
	else if (m_fifo_count < std::size(m_fifo))
	{
		m_fifo[(m_fifo_rp + m_fifo_count) % std::size(m_fifo)] = data;
		m_fifo_count++;
	}
}


// one instruction of the engine, as voxeng.py's Engine.step and avox_engine.sv
void atarivox_voice_device::step()
{
	u64 const w = (m_pc < std::size(AVOX_IROM)) ? AVOX_IROM[m_pc] : 0;
	unsigned const op = BIT(w, 35, 5);
	unsigned const d = BIT(w, 29, 6);
	u32 const a = m_mem[BIT(w, 23, 6)];
	u32 const b = m_mem[BIT(w, 17, 6)];
	u16 const imm = BIT(w, 0, 16);
	u32 const a_imm = a + u32(s32(s16(imm)));
	u64 const last = m_clk + CLK_PER_INSTR - 1;

	// it reads at its last clock, so it sees a tick raised up to the clock before
	if (wraps(m_clk, last - 1))
		m_flags |= FLAG_TICK;

	u16 npc = m_pc + 1;
	bool write = true;
	u32 res = 0;
	switch (op)
	{
	case OP_ADD:  res = a + b; break;
	case OP_SUB:  res = a - b; break;
	case OP_AND:  res = a & b; break;
	case OP_OR:   res = a | b; break;
	case OP_XOR:  res = a ^ b; break;
	case OP_ADDI: res = a_imm; break;
	case OP_ANDI: res = a & imm; break;
	case OP_ORI:  res = a | imm; break;
	case OP_LUI:  res = u32(imm) << 16; break;
	case OP_MUL:
		{
			static constexpr unsigned SHIFTS[4] = { 0, 7, 12, 16 };
			s64 const p = s64(util::sext(a, 27)) * util::sext(b, 27);
			res = u32(p >> SHIFTS[imm & 3]);
		}
		break;
	case OP_HI12: res = a >> 20; break;
	case OP_SIN:  res = drom(BIT(a, 22, 9)); break;
	case OP_LDR:  res = drom(a_imm & 0xfff); break;
	case OP_LDRS: res = util::sext(drom(a_imm & 0xfff), 20); break;
	case OP_LDM:  res = m_mem[a_imm & 0xff]; break;
	case OP_STM:
		write = false;
		if (a_imm & 0xff)
			m_mem[a_imm & 0xff] = b;
		break;
	case OP_BEQ:
	case OP_BNE:
	case OP_BLT:
	case OP_BGE:
	case OP_BLTU:
		{
			write = false;
			bool take = false;
			switch (op)
			{
			case OP_BEQ:  take = a == b; break;
			case OP_BNE:  take = a != b; break;
			case OP_BLT:  take = s32(a) < s32(b); break;
			case OP_BGE:  take = s32(a) >= s32(b); break;
			case OP_BLTU: take = a < b; break;
			}
			if (take)
				npc = imm;
		}
		break;
	case OP_JAL:
		res = m_pc + 1;
		npc = imm;
		break;
	case OP_JR:
		write = false;
		npc = a;
		break;
	case OP_IN:
		switch (imm & 3)
		{
		case 0:     // the flags, which reading clears
			res = m_flags;
			m_flags = 0;
			break;
		case 1:     // the next byte of the buffer
			if (m_fifo_count)
			{
				res = m_fifo[m_fifo_rp];
				m_fifo_rp = (m_fifo_rp + 1) % std::size(m_fifo);
				m_fifo_count--;
			}
			break;
		case 2:     // noise
			res = m_lfsr;
			m_lfsr = (m_lfsr >> 1) | ((BIT(m_lfsr, 0) ^ BIT(m_lfsr, 2) ^ BIT(m_lfsr, 3) ^ BIT(m_lfsr, 5)) << 15);
			break;
		case 3:
			res = m_fifo_count ? 1 : 0;
			break;
		}
		break;
	case OP_OUT:
		write = false;
		m_sample = s16(u16(a));
		break;
	default:    // unused codes write 0
		break;
	}
	if (write && d)
		m_mem[d] = res;

	// a tick on the last clock lands after this instruction's read and clear
	if (wraps(last, last))
		m_flags |= FLAG_TICK;

	m_pc = npc & 0x3ff;
	m_clk += CLK_PER_INSTR;
}

void atarivox_voice_device::sound_stream_update(sound_stream &stream)
{
	for (int i = 0; i < stream.samples(); i++)
	{
		// each sample is a period of the tick divider: run to its end and take the held output
		u64 const end = (m_clk / TICK_DIV + 1) * TICK_DIV;
		if (m_running)
		{
			while (m_clk < end)
				step();
		}
		else
		{
			m_clk = end;
		}

		// a TIA channel at full volume reaches 15,360 here and 21,844 in the core, where the voice is
		// mixed at x8 ("Normal"): the same balance
		stream.put_int(0, i, m_sample * 45, 32768 * 8);
	}
}


//**************************************************************************
//  the AtariVox on a controller port
//**************************************************************************

vcs_atarivox_device::vcs_atarivox_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	vcs_savekey_device(mconfig, VCS_ATARIVOX, tag, owner, clock),
	m_voice(*this, "voice")
{
}

void vcs_atarivox_device::device_add_mconfig(machine_config &config)
{
	vcs_savekey_device::device_add_mconfig(config);

	// the AtariVox has its own audio jack
	SPEAKER(config, "speaker").front_center();
	ATARIVOX_VOICE(config, m_voice, atarivox_voice_device::CLOCK).add_route(ALL_OUTPUTS, "speaker", 1.0);
}

void vcs_atarivox_device::vcs_joy_w(uint8_t data)
{
	vcs_savekey_device::vcs_joy_w(data);
	m_voice->line_w(BIT(data, 0));
}

uint8_t vcs_atarivox_device::vcs_joy_r()
{
	return vcs_savekey_device::vcs_joy_r() & (m_voice->ready_r() ? 0xff : ~0x02);
}
