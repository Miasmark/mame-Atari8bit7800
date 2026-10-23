// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/***************************************************************************

 Atari 2600 cart Starpath Supercharger (Cart + Tape drive!)



 From kevtris notes ( http://blog.kevtris.org/blogfiles/Atari%202600%20Mappers.txt ):


 - Control register [0x1ff8]

       7       0
       ---------
 1FF8: DDDB BBWE

 D: write delay (see below)
 B: bankswitching mode (see below)
 W: RAM write enable (1 = enabled, 0 = disabled)
 E: ROM power enable (0 = enabled, 1 = turn off ROM)

 - Audio input register [0x1ff9]

        7       0
        ---------
 1FF9:  0000 000A

 A: Supercharger audio data.  0 = low input, 1 = high input.

 Tape images (.a26 files of n * 8448 bytes, one 8448 byte record per tape
 load: 8K of 256 byte blocks, then the 8 byte load header at 0x2000, the
 RAM location of each block at 0x2010 and the block checksums at 0x2040,
 the layout formats/a26_cas.cpp turns into audio) can also be loaded in
 place of the BIOS ROM. As the BIOS ROM is then missing, the 2K ROM bank is
 replaced with a small loader of our own that copies a load straight into
 the RAM and starts it the way the BIOS does: writing the header's control
 byte to 0x1ff8 and jumping to the header's start address. Multiload games
 ask the BIOS for the next part by storing its load number at 0xfa and
 jumping to 0xf800, which the loader also handles.

***************************************************************************/


#include "emu.h"
#include "scharger.h"
#include "formats/a26_cas.h"

//#define VERBOSE (LOG_GENERAL)
#include "logmacro.h"


DEFINE_DEVICE_TYPE(A26_ROM_SUPERCHARGER, a26_rom_ss_device, "a2600_ss", "Atari 2600 ROM Cart Supercharger")


a26_rom_ss_device::a26_rom_ss_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	a26_rom_base_device(mconfig, A26_ROM_SUPERCHARGER, tag, owner, clock),
	m_cassette(*this, "cassette"),
	m_data(0),
	m_write_delay(0),
	m_ram_write_enabled(0),
	m_rom_enabled(0),
	m_last_address_bus(0),
	m_address_bus_changes(0),
	m_space(nullptr),
	m_tape_loads(0),
	m_current_load(0)
{
}

//-------------------------------------------------
//  mapper specific start/reset
//-------------------------------------------------

void a26_rom_ss_device::device_start()
{
	save_item(NAME(m_base_banks));
	save_item(NAME(m_data));
	save_item(NAME(m_write_delay));
	save_item(NAME(m_ram_write_enabled));
	save_item(NAME(m_rom_enabled));
	save_item(NAME(m_last_address_bus));
	save_item(NAME(m_address_bus_changes));
	save_item(NAME(m_loader));
	save_item(NAME(m_current_load));
}

void a26_rom_ss_device::device_reset()
{
	// banks = 0, 1, 2 are 2K chunk of RAM (of the available 6K), banks = 3 is ROM
	m_base_banks[0] = 2;
	m_base_banks[1] = 3;
	m_ram_write_enabled = false;
	m_data = 0;
	m_write_delay = 0;
	m_rom_enabled = true;
	m_last_address_bus = 0;
	m_address_bus_changes = RAM_WRITE_TIME + 1;
}


void a26_rom_ss_device::device_add_mconfig(machine_config &config)
{
	CASSETTE(config, m_cassette);
	m_cassette->set_formats(a26_cassette_formats);
	m_cassette->set_default_state(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED);
	m_cassette->set_interface("a2600_cass");
}

void a26_rom_ss_device::install_memory_handlers(address_space *space)
{
	m_space = space;
	if (get_rom_size() >= TAPE_LOAD_SIZE && !(get_rom_size() % TAPE_LOAD_SIZE))
		setup_tape_loader();

	space->install_read_handler(0x1000, 0x17ff, read8sm_delegate(*this, FUNC(a26_rom_ss_device::read_lo)));
	space->install_read_handler(0x1800, 0x1fff, read8sm_delegate(*this, FUNC(a26_rom_ss_device::read_hi)));
	space->install_read_handler(0x1ff9, 0x1ff9, read8sm_delegate(*this, FUNC(a26_rom_ss_device::read_cass)));
	space->install_readwrite_tap(0x0000, 0x1fff, "watch",
		[this] (offs_t address, u8 &, u8) { if (!machine().side_effects_disabled()) tap(address); },
		[this] (offs_t address, u8 &, u8) { if (!machine().side_effects_disabled()) tap(address); });
}

void a26_rom_ss_device::tap(offs_t address)
{
	if (address == 0x1ff8)
	{
		LOG("write control register %02X\n", m_data);
		m_write_delay = m_data >> 5;
		m_ram_write_enabled = BIT(m_data, 1);
		m_rom_enabled = !BIT(m_data, 0);

		// prevent spurious RAM write
		m_address_bus_changes = RAM_WRITE_TIME + 1;

		// handle bankswitch
		switch (m_data & 0x1c)
		{
			case 0x00:
				m_base_banks[0] = 2;
				m_base_banks[1] = 3;
				break;
			case 0x04:
				m_base_banks[0] = 0;
				m_base_banks[1] = 3;
				break;
			case 0x08:
				m_base_banks[0] = 2;
				m_base_banks[1] = 0;
				break;
			case 0x0c:
				m_base_banks[0] = 0;
				m_base_banks[1] = 2;
				break;
			case 0x10:
				m_base_banks[0] = 2;
				m_base_banks[1] = 3;
				break;
			case 0x14:
				m_base_banks[0] = 1;
				m_base_banks[1] = 3;
				break;
			case 0x18:
				m_base_banks[0] = 2;
				m_base_banks[1] = 1;
				break;
			case 0x1c:
				m_base_banks[0] = 1;
				m_base_banks[1] = 2;
				break;
		}
	}
	if (m_address_bus_changes <= RAM_WRITE_TIME && m_last_address_bus != address)
		m_address_bus_changes++;
	m_last_address_bus = address;
}

uint8_t a26_rom_ss_device::read_lo(offs_t offset)
{
	if (!machine().side_effects_disabled())
	{
		if (m_ram_write_enabled)
		{
			if (m_address_bus_changes == RAM_WRITE_TIME)
			{
				LOG("%s: RAM write offset %04X, data %02X\n", machine().describe_context(), offset + (m_base_banks[0] * 0x800), m_data);
				m_ram[offset + (m_base_banks[0] * 0x800)] = m_data;
			}
			else if (offset < 0x0100)
			{
				m_data = offset;
				m_address_bus_changes = 0;
			}
		}
		else if (offset < 0x0100)
		{
			m_data = offset;
			m_address_bus_changes = 0;
		}
	}
	return m_ram[offset + (m_base_banks[0] * 0x800)];
}

uint8_t a26_rom_ss_device::read_hi(offs_t offset)
{
	if (!machine().side_effects_disabled())
	{
		if (m_ram_write_enabled)
		{
			if (m_address_bus_changes == RAM_WRITE_TIME)
			{
				if (m_base_banks[1] != 3)
				{
					LOG("%s: RAM write offset %04X, data %02X\n", machine().describe_context(), offset + (m_base_banks[1] * 0x800), m_data);
					m_ram[offset + (m_base_banks[1] * 0x800)] = m_data;
				}
			}
		}
	}
	if (m_base_banks[1] != 3)
		return m_ram[offset + (m_base_banks[1] * 0x800)];
	else if (!m_rom_enabled)
		return 0xff;
	else if (!m_tape_loads)
		return m_rom[offset];

	// our own loader in place of the BIOS
	if (!machine().side_effects_disabled() && (offset == LOADER_LOAD_REQUESTED || offset == LOADER_LOAD_FIRST))
		tape_load(offset == LOADER_LOAD_FIRST);
	return m_loader[offset];
}


//-------------------------------------------------
//  tape image loader
//-------------------------------------------------

void a26_rom_ss_device::setup_tape_loader()
{
	m_tape_loads = get_rom_size() / TAPE_LOAD_SIZE;
	std::fill(std::begin(m_loader), std::end(m_loader), 0);

	// f800: the BIOS entry multiload games jump to, with the wanted load number at 00fa
	// f806: reset entry, loads the first load of the image
	static constexpr uint8_t LOADER[] =
	{
		0xad, 0xf0, 0xff,           // f800: LDA $FFF0      copy the load asked for at $FA into RAM
		0x4c, 0x09, 0xf8,           //       JMP $F809
		0xad, 0xf1, 0xff,           // f806: LDA $FFF1      copy the first load into RAM
		0x78,                       // f809: SEI
		0xd8,                       //       CLD
		0xa2, 0xff,                 //       LDX #$FF
		0x9a,                       //       TXS
		0xa9, 0x01,                 //       LDA #$01       leave the RIOT timer run out, as after a real
		0x8d, 0x97, 0x02,           //       STA $0297      load (some games first wait for it), and at
		                            //                      1024T: read again, it counts on at that
		                            //                      rate (Rabbit Transit waits for INTIM to
		                            //                      read 0 once a line, which 1T would skip)
		// leave "CMP $FFF8 / JMP start" in RAM at $FA-$FF, as the ROM goes away
		// when the control register is written
		0xa9, 0xcd, 0x85, 0xfa,     //       LDA #$CD / STA $FA
		0xa9, 0xf8, 0x85, 0xfb,     //       LDA #$F8 / STA $FB
		0xa9, 0xff, 0x85, 0xfc,     //       LDA #$FF / STA $FC
		0xa9, 0x4c, 0x85, 0xfd,     //       LDA #$4C / STA $FD
		0xa9, 0x00, 0x85, 0xfe,     //       LDA #<start / STA $FE (patched)
		0xa9, 0x00, 0x85, 0xff,     //       LDA #>start / STA $FF (patched)
		0xa9, 0x00, 0x85, 0x80,     //       LDA #control / STA $80 (patched): games read the
		                            //       current control byte back from $80
		0xcd, 0x00, 0xf0,           //       CMP $F0xx      latch the control byte (patched)
		0x4c, 0xfa, 0x00            //       JMP $00FA
	};
	std::copy(std::begin(LOADER), std::end(LOADER), std::begin(m_loader));

	// reset and break vectors
	m_loader[0x7fc] = m_loader[0x7fe] = 0x06;
	m_loader[0x7fd] = m_loader[0x7ff] = 0xf8;
}

void a26_rom_ss_device::tape_load(bool first)
{
	unsigned load = 0;
	if (!first)
	{
		// the BIOS looks for the load with the multiload number the game stored at $FA
		uint8_t wanted;
		{
			auto dis = machine().disable_side_effects();
			wanted = m_space->read_byte(0xfa);
		}
		load = m_tape_loads;
		for (unsigned i = 0; i < m_tape_loads; i++)
			if (m_rom[i * TAPE_LOAD_SIZE + 0x2005] == wanted)
				load = i;
		if (load == m_tape_loads)
		{
			// the BIOS would wait for it forever: play the next load on the tape instead
			load = (m_current_load + 1) % m_tape_loads;
			logerror("tape image has no load %u, loading load %u instead\n", wanted, load);
		}
	}
	m_current_load = load;

	uint8_t const *const data = &m_rom[load * TAPE_LOAD_SIZE];
	uint8_t const *const header = &data[0x2000];
	unsigned const blocks = std::min<unsigned>(header[3], 0x20);
	for (unsigned i = 0; i < blocks; i++)
	{
		// block location: 000B BBRR, B = 256 byte block, R = 2K RAM bank (3 would be the ROM)
		uint8_t const location = data[0x2010 + i];
		if ((location & 0x03) == 0x03)
		{
			logerror("load %u: block %u has an invalid location %02x\n", load, i, location);
			continue;
		}
		std::copy_n(&data[i * 0x100], 0x100, &m_ram[(location & 0x03) * 0x800 + ((location >> 2) & 0x07) * 0x100]);
	}
	LOG("tape load %u (multiload %u): start %02X%02X, control %02X, %u blocks\n", load, header[5], header[1], header[0], header[2], blocks);

	m_loader[LOADER_START_LO] = header[0];
	m_loader[LOADER_START_HI] = header[1];
	m_loader[LOADER_CONTROL] = header[2];
	m_loader[LOADER_CONTROL_RAM] = header[2];

	// prevent spurious RAM writes
	m_address_bus_changes = RAM_WRITE_TIME + 1;
}

uint8_t a26_rom_ss_device::read_cass(offs_t offset)
{
	// prevent spurious RAM write
	m_address_bus_changes = RAM_WRITE_TIME + 1;

	if (m_cassette->input() < 0)
		return 0x00;
	else
		return 0x01;
}
