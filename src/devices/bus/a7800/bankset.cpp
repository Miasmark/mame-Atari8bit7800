// license:BSD-3-Clause
// copyright-holders:Mike Saarna
/***********************************************************************************************************

 Atari 7800 bankset carts -- see bankset.h

***********************************************************************************************************/

#include "emu.h"
#include "bankset.h"

#include "speaker.h"

DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_SG, a78_bankset_sg_device, "a78_bankset_sg", "Atari 7800 Bankset SG Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_SG_POK800, a78_bankset_sg_p800_device, "a78_bankset_sg_p800", "Atari 7800 Bankset SG Pokey800 Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_SG_BANKRAM, a78_bankset_sg_bankram_device, "a78_bankset_sg_bankram", "Atari 7800 Bankset SG BankRAM Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_SG_BANKRAM_POK800, a78_bankset_sg_bankram_p800_device, "a78_bankset_sg_bankram_p800", "Atari 7800 Bankset SG BankRAM Pokey800 Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_SG_RAM, a78_bankset_sg_ram_device, "a78_bankset_sg_ram", "Atari 7800 Bankset SG RAM Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_SG_RAM_POK800, a78_bankset_sg_ram_p800_device, "a78_bankset_sg_ram_p800", "Atari 7800 Bankset SG RAM Pokey800 Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_ROM, a78_bankset_rom_device, "a78_bankset_rom", "Atari 7800 Bankset Rom Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_POK800, a78_bankset_rom_p800_device, "a78_bankset_rom_p800", "Atari 7800 Bankset Rom Pokey800 Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_POK4000, a78_bankset_rom_p4000_device, "a78_bankset_rom_p4000", "Atari 7800 Bankset Rom Pokey4000 Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_BANKRAM, a78_bankset_bankram_device, "a78_bankset_bankram", "Atari 7800 Bankset BankRAM Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_BANKRAM_POK800, a78_bankset_bankram_p800_device, "a78_bankset_bankram_p800", "Atari 7800 Bankset BankRAM Pokey800 Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_52K, a78_bankset_rom_52k_device, "a78_bankset_rom_52k", "Atari 7800 Bankset Rom 52K Cart")
DEFINE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_52K_POK4000, a78_bankset_rom_52k_p4000_device, "a78_bankset_rom_52k_p4000", "Atari 7800 Bankset Rom 52K Pokey4000 Cart")


a78_bankset_sg_device::a78_bankset_sg_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: a78_rom_sg_device(mconfig, type, tag, owner, clock)
{
}

a78_bankset_sg_device::a78_bankset_sg_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_sg_device(mconfig, A78_ROM_BANKSET_SG, tag, owner, clock)
{
}

a78_bankset_sg_p800_device::a78_bankset_sg_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_pokey800_base<a78_bankset_sg_device>(mconfig, A78_ROM_BANKSET_SG_POK800, tag, owner, clock)
{
}

a78_bankset_sg_bankram_device::a78_bankset_sg_bankram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: a78_rom_sg_device(mconfig, type, tag, owner, clock)
{
}

a78_bankset_sg_bankram_device::a78_bankset_sg_bankram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_sg_bankram_device(mconfig, A78_ROM_BANKSET_SG_BANKRAM, tag, owner, clock)
{
}

a78_bankset_sg_bankram_p800_device::a78_bankset_sg_bankram_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_pokey800_base<a78_bankset_sg_bankram_device>(mconfig, A78_ROM_BANKSET_SG_BANKRAM_POK800, tag, owner, clock)
{
}

a78_bankset_sg_ram_device::a78_bankset_sg_ram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: a78_rom_sg_device(mconfig, type, tag, owner, clock)
{
}

a78_bankset_sg_ram_device::a78_bankset_sg_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_sg_ram_device(mconfig, A78_ROM_BANKSET_SG_RAM, tag, owner, clock)
{
}

a78_bankset_sg_ram_p800_device::a78_bankset_sg_ram_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_pokey800_base<a78_bankset_sg_ram_device>(mconfig, A78_ROM_BANKSET_SG_RAM_POK800, tag, owner, clock)
{
}

a78_bankset_rom_device::a78_bankset_rom_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: a78_rom_device(mconfig, type, tag, owner, clock)
{
}

a78_bankset_rom_device::a78_bankset_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_rom_device(mconfig, A78_ROM_BANKSET_ROM, tag, owner, clock)
{
}

a78_bankset_rom_p800_device::a78_bankset_rom_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_pokey800_base<a78_bankset_rom_device>(mconfig, A78_ROM_BANKSET_ROM_POK800, tag, owner, clock)
{
}

a78_bankset_rom_p4000_device::a78_bankset_rom_p4000_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_rom_device(mconfig, A78_ROM_BANKSET_ROM_POK4000, tag, owner, clock)
	, m_pokey4000(*this, "pokey4000")
{
}

a78_bankset_bankram_device::a78_bankset_bankram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: a78_rom_device(mconfig, type, tag, owner, clock)
{
}

a78_bankset_bankram_device::a78_bankset_bankram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_bankram_device(mconfig, A78_ROM_BANKSET_BANKRAM, tag, owner, clock)
{
}

a78_bankset_bankram_p800_device::a78_bankset_bankram_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_pokey800_base<a78_bankset_bankram_device>(mconfig, A78_ROM_BANKSET_BANKRAM_POK800, tag, owner, clock)
{
}

a78_bankset_rom_52k_device::a78_bankset_rom_52k_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: a78_rom_device(mconfig, type, tag, owner, clock)
{
}

a78_bankset_rom_52k_device::a78_bankset_rom_52k_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_rom_52k_device(mconfig, A78_ROM_BANKSET_ROM_52K, tag, owner, clock)
{
}

a78_bankset_rom_52k_p4000_device::a78_bankset_rom_52k_p4000_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: a78_bankset_rom_52k_device(mconfig, A78_ROM_BANKSET_ROM_52K_POK4000, tag, owner, clock)
	, m_pokey4000(*this, "pokey4000")
{
}


void a78_bankset_rom_p4000_device::device_add_mconfig(machine_config &config)
{
	SPEAKER(config, "pokey_4000").front_center();

	POKEY(config, m_pokey4000, DERIVED_CLOCK(1, 1)).add_route(ALL_OUTPUTS, "pokey_4000", 1.00);
}

void a78_bankset_rom_52k_p4000_device::device_add_mconfig(machine_config &config)
{
	SPEAKER(config, "pokey_4000").front_center();

	POKEY(config, m_pokey4000, DERIVED_CLOCK(1, 1)).add_route(ALL_OUTPUTS, "pokey_4000", 1.00);
}


uint8_t a78_bankset_sg_device::read_40xx(offs_t offset)
{
	// Sally sees the first half of the image, MARIA (m_dmaactive) the second
	uint32_t const half_banks = m_bank_mask / 2;
	uint32_t const base = m_dmaactive ? ((half_banks + 1) * 0x4000) : 0;

	if (offset < 0x4000)
		return m_rom[offset + (half_banks * 0x4000) - 0x4000 + base]; // second last bank
	else if (offset < 0x8000)
		return m_rom[(offset & 0x3fff) + (m_bank * 0x4000) + base];
	else
		return m_rom[(offset & 0x3fff) + (half_banks * 0x4000) + base]; // last bank
}

void a78_bankset_sg_device::write_40xx(offs_t offset, uint8_t data)
{
	if (offset >= 0x4000 && offset < 0x8000)
		m_bank = (data & 0x0f) & (m_bank_mask / 2); // up to 2 * 256K banksets
}

uint8_t a78_bankset_sg_p800_device::read_40xx(offs_t offset)
{
	// nothing at $4000-$7fff (can we do better?)
	if (offset < 0x4000)
		return 0xff;
	return a78_bankset_sg_device::read_40xx(offset);
}

uint8_t a78_bankset_sg_bankram_device::read_40xx(offs_t offset)
{
	// Sally sees the first half of the image, MARIA (m_dmaactive) the second
	uint32_t const half_banks = m_bank_mask / 2;
	uint32_t const base = m_dmaactive ? ((half_banks + 1) * 0x4000) : 0;

	if (offset < 0x4000)
		return m_ram[m_dmaactive ? (offset + 0x4000) : offset];
	else if (offset < 0x8000)
		return m_rom[(offset & 0x3fff) + (m_bank * 0x4000) + base];
	else
		return m_rom[(offset & 0x3fff) + (half_banks * 0x4000) + base]; // last bank
}

void a78_bankset_sg_bankram_device::write_40xx(offs_t offset, uint8_t data)
{
	if (offset < 0x4000)
		m_ram[offset] = data; // MARIA can only read, so this has to be Sally's bank RAM
	else if (offset < 0x8000)
		m_bank = (data & 0x0f) & (m_bank_mask / 2); // up to 2 * 256K banksets
	else
		m_ram[offset - 0x4000] = data; // Sally writes MARIA's RAM
}

uint8_t a78_bankset_sg_ram_device::read_40xx(offs_t offset)
{
	// Sally sees the first half of the image, MARIA (m_dmaactive) the second; the RAM is the same for both
	uint32_t const half_banks = m_bank_mask / 2;
	uint32_t const base = m_dmaactive ? ((half_banks + 1) * 0x4000) : 0;

	if (offset < 0x4000)
		return m_ram[offset];
	else if (offset < 0x8000)
		return m_rom[(offset & 0x3fff) + (m_bank * 0x4000) + base];
	else
		return m_rom[(offset & 0x3fff) + (half_banks * 0x4000) + base]; // last bank
}

void a78_bankset_sg_ram_device::write_40xx(offs_t offset, uint8_t data)
{
	if (offset < 0x4000)
		m_ram[offset] = data;
	else if (offset < 0x8000)
		m_bank = (data & 0x0f) & (m_bank_mask / 2); // up to 2 * 256K banksets
}

uint8_t a78_bankset_rom_device::read_40xx(offs_t offset)
{
	// the image is two halves, each ending at $FFFF; Sally sees the first, MARIA the second
	uint32_t const addrstart = 0xc000 - (m_rom_size / 2);

	if (offset < addrstart)
		return 0xff;
	return m_rom[offset - addrstart + (m_dmaactive ? (m_rom_size / 2) : 0)];
}

void a78_bankset_rom_p4000_device::write_40xx(offs_t offset, uint8_t data)
{
	// the POKEY's 16 registers at $4000-$400f
	if (offset < 0x10)
		m_pokey4000->write(offset, data);
}

uint8_t a78_bankset_bankram_device::read_40xx(offs_t offset)
{
	// the image is two halves, each ending at $FFFF; Sally sees the first, MARIA the second
	uint32_t const addrstart = 0xc000 - (m_rom_size / 2);

	if (offset < 0x4000)
		return m_ram[m_dmaactive ? (offset + 0x4000) : offset];
	if (offset < addrstart)
		return 0xff;
	return m_rom[offset - addrstart + (m_dmaactive ? (m_rom_size / 2) : 0)];
}

void a78_bankset_bankram_device::write_40xx(offs_t offset, uint8_t data)
{
	if (offset < 0x4000)
		m_ram[offset] = data; // MARIA can only read, so this has to be Sally's bank RAM
	else if (offset >= 0x8000)
		m_ram[offset - 0x4000] = data; // Sally writes MARIA's RAM
}

uint8_t a78_bankset_rom_52k_device::read_40xx(offs_t offset)
{
	return m_rom[offset + 0x1000 + (m_dmaactive ? 0xd000 : 0)];
}

uint8_t a78_bankset_rom_52k_device::read_30xx(offs_t offset)
{
	return m_rom[offset + (m_dmaactive ? 0xd000 : 0)];
}

void a78_bankset_rom_52k_p4000_device::write_40xx(offs_t offset, uint8_t data)
{
	// the POKEY's registers repeat through $4000-$7fff
	if (offset < 0x4000)
		m_pokey4000->write(offset & 0x0f, data);
}
