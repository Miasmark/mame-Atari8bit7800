// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/***********************************************************************************************************


 Mattel Intellivision Entertainment Computer System expansion emulation

 TODO:
   - Page writes are decoded only for the 4K blocks the ECS ROMs and World Series
     Major League Baseball use ($2000, $7000, $E000, $F000), not for every block
     (http://spatula-city.org/~im14u2c/intv/tech/ecs.html)

 ***********************************************************************************************************/


#include "emu.h"
#include "ecs.h"
#include "speaker.h"


//-------------------------------------------------
//  intv_ecs_device - constructor
//-------------------------------------------------

DEFINE_DEVICE_TYPE(INTV_ROM_ECS, intv_ecs_device, "intv_ecs", "Intellivision ECS Expansion")

intv_ecs_device::intv_ecs_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	intv_rom_device(mconfig, INTV_ROM_ECS, tag, owner, clock),
	m_snd(*this, "ay8917"),
	m_subslot(*this, "subslot"),
	m_voice_enabled(false),
	m_ramd0_enabled(false),
	m_ram88_enabled(false)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void intv_ecs_device::device_start()
{
	// if the ECS is mounted directly in the system, use device rom and alloc RAM
	if (m_rom == nullptr)
	{
		std::string region_tag;
		m_rom = memregion(region_tag.assign(tag()).append(":ecs").c_str())->base();
	}
	if (m_ram.empty())
	{
		m_ram.resize(0x800);
	}

	save_item(NAME(m_bank_base));
}

void intv_ecs_device::device_reset()
{
	memset(m_bank_base, 0, sizeof(m_bank_base));
}

void intv_ecs_device::late_subslot_setup()
{
	switch (m_subslot->get_type())
	{
		case INTV_RAM:
			m_ramd0_enabled = true;
			break;
		case INTV_GFACT:
			m_ram88_enabled = true;
			break;
		case INTV_VOICE:
			m_voice_enabled = true;
			m_subslot->late_subslot_setup();
			break;
		case INTV_ECS:
			printf("WARNING: You cannot connect serially multiple ECS units.\n");
			printf("WARNING: Emulation will likely misbehave.\n");
			break;
		case INTV_KEYCOMP:
			printf("WARNING: You cannot connect the Keyboard component to the ECS unit.\n");
			printf("WARNING: Emulation will likely misbehave.\n");
			break;
	}
}

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------


void intv_ecs_device::device_add_mconfig(machine_config &config)
{
	SPEAKER(config, "mono_ecs").front_center();

	AY8917(config, m_snd, XTAL(3'579'545)/2);
	m_snd->port_a_read_callback().set("ctrl_port", FUNC(intvecs_control_port_device::porta_r));
	m_snd->port_b_read_callback().set("ctrl_port", FUNC(intvecs_control_port_device::portb_r));
	m_snd->port_a_write_callback().set("ctrl_port", FUNC(intvecs_control_port_device::porta_w));
	m_snd->port_b_write_callback().set("ctrl_port", FUNC(intvecs_control_port_device::portb_w));
	m_snd->add_route(ALL_OUTPUTS, "mono_ecs", 0.33);

	INTVECS_CONTROL_PORT(config, "ctrl_port", intvecs_control_port_devices, "keybd");
	INTV_CART_SLOT(config, m_subslot, intv_cart, nullptr);
}


ROM_START( ecs )
	ROM_REGION( 0x20000, "ecs", ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "ecs_rom.20", 0x04000, 0x2000, CRC(5f9d05e5) SHA1(083a3e7405b8f8b4b8a5003ca9c31b8d824b535c))
	ROM_LOAD16_WORD_SWAP( "ecs_rom.70", 0x0e000, 0x2000, CRC(46bb1f48) SHA1(eda44a8476fdada1ae90fba0d0287611e2efa074))
	ROM_LOAD16_WORD_SWAP( "ecs_rom.e0", 0x1c000, 0x2000, CRC(c2ebcd90) SHA1(b3c14955f56c57e6f0d8fbb695771946cfcf6582))
ROM_END

const tiny_rom_entry *intv_ecs_device::device_rom_region() const
{
	return ROM_NAME( ecs );
}


/*-------------------------------------------------
 Paged ROM handling

 A write of $xA5y to $xFFF selects page y of the 4K
 at $x000. The ECS ROMs answer on their own pages
 ($2000 and $E000 on 1, $7000 on 0) and the
 cartridge, which ignores the pages, on the others;
 World Series Major League Baseball has two pages
 at $F000.
 -------------------------------------------------*/

uint16_t intv_ecs_device::read_rom20(offs_t offset)
{
	if (m_bank_base[2] == 1)
		return INTV_ROM16_READ(offset + 0x2000);
	else
		return m_subslot->read_rom20(offset);
}

uint16_t intv_ecs_device::read_rom70(offs_t offset)
{
	if (m_bank_base[7] == 0)
		return INTV_ROM16_READ(offset + 0x7000);
	else
		return m_subslot->read_rom70(offset);
}

uint16_t intv_ecs_device::read_rome0(offs_t offset)
{
	if (m_bank_base[14] == 1)
		return INTV_ROM16_READ(offset + 0xe000);
	else    // if WSMLB is loaded, it shall go here, otherwise 0xffff
		return m_subslot->read_rome0(offset);
}

uint16_t intv_ecs_device::read_romf0(offs_t offset)
{
	// only WSMLB has a page 1 here
	if (m_bank_base[15] == 1)
		return m_subslot->read_romf0(offset + 0x1000);
	else
		return m_subslot->read_romf0(offset);
}

void intv_ecs_device::select_page(int segment, offs_t offset, uint16_t data)
{
	if (offset == 0xfff && (data & 0xfff0) == ((segment << 12) | 0x0a50))
		m_bank_base[segment] = data & 0x000f;
}


/*-------------------------------------------------
 read_uart - the cassette and printer UART, $E0-$E3:
 its status ($E0) says the transmitter is ready,
 as a real ECS with no tape or printer does, so a
 tape save runs to its end; what is sent is lost
 -------------------------------------------------*/

uint16_t intv_ecs_device::read_uart(offs_t offset)
{
	return (offset == 0) ? 0x0002 : 0x0000;
}


/*-------------------------------------------------
 read_audio
 -------------------------------------------------*/

uint16_t intv_ecs_device::read_ay(offs_t offset)
{
	return 0xff00 | m_snd->read(offset);
}

/*-------------------------------------------------
 write_audio
 -------------------------------------------------*/

void intv_ecs_device::write_ay(offs_t offset, uint16_t data)
{
	return m_snd->write(offset, data & 0x00ff);
}


uint16_t intv_ecs_device::read_rom80(offs_t offset)
{
	if (m_ram88_enabled && offset >= 0x800)
		return m_subslot->read_ram(offset & 0x7ff);
	else
		return m_subslot->read_rom80(offset);
}


uint16_t intv_ecs_device::read_romd0(offs_t offset)
{
	if (m_ramd0_enabled && offset < 0x800)
		return m_subslot->read_ram(offset);
	else
		return m_subslot->read_romd0(offset);
}


void intv_ecs_device::write_rom20(offs_t offset, uint16_t data)
{
	select_page(0x2, offset, data);
}

void intv_ecs_device::write_rom70(offs_t offset, uint16_t data)
{
	select_page(0x7, offset, data);
}

void intv_ecs_device::write_rome0(offs_t offset, uint16_t data)
{
	select_page(0xe, offset, data);
}

void intv_ecs_device::write_romf0(offs_t offset, uint16_t data)
{
	select_page(0xf, offset, data);
}
