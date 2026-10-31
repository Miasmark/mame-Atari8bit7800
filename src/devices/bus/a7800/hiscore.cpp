// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/***********************************************************************************************************

 A7800 HighScore passthrough cart emulation

 Its RAM ($1000-$17FF) and ROM ($3000-$3FFF) are the CPU's only: MARIA, there and everywhere else, sees
 the game plugged into it, and so does the CPU everywhere else.

***********************************************************************************************************/


#include "emu.h"
#include "hiscore.h"
#include "a78_carts.h"


//-------------------------------------------------
//  constructor
//-------------------------------------------------

DEFINE_DEVICE_TYPE(A78_HISCORE, a78_hiscore_device, "a78_hiscore", "Atari 7800 High Score Cart")


a78_hiscore_device::a78_hiscore_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_rom_device(mconfig, A78_HISCORE, tag, owner, clock)
	, m_hscslot(*this, "hsc_slot")
{
}


void a78_hiscore_device::device_add_mconfig(machine_config &config)
{
	A78_CART_SLOT(config, m_hscslot, DERIVED_CLOCK(1, 1), a7800_cart, nullptr);
}


/*-------------------------------------------------
 mapper specific handlers
 -------------------------------------------------*/

uint8_t a78_hiscore_device::read_10xx(offs_t offset)
{
	return m_dmaactive ? m_hscslot->read_10xx(offset) : m_nvram[offset];
}

void a78_hiscore_device::write_10xx(offs_t offset, uint8_t data)
{
	m_nvram[offset] = data;
}

uint8_t a78_hiscore_device::read_30xx(offs_t offset)
{
	return m_dmaactive ? m_hscslot->read_30xx(offset) : m_rom[offset];
}

uint8_t a78_hiscore_device::read_08xx(offs_t offset)
{
	return m_hscslot->read_08xx(offset);
}

void a78_hiscore_device::write_08xx(offs_t offset, uint8_t data)
{
	m_hscslot->write_08xx(offset, data);
}

void a78_hiscore_device::set_dma_active(bool active)
{
	m_dmaactive = active;
	m_hscslot->set_dma_active(active);
}

uint8_t a78_hiscore_device::read_04xx(offs_t offset)
{
	return m_hscslot->read_04xx(offset);
}

void a78_hiscore_device::write_04xx(offs_t offset, uint8_t data)
{
	m_hscslot->write_04xx(offset, data);
}

uint8_t a78_hiscore_device::read_40xx(offs_t offset)
{
	return m_hscslot->read_40xx(offset);
}

void a78_hiscore_device::write_40xx(offs_t offset, uint8_t data)
{
	m_hscslot->write_40xx(offset, data);
}
