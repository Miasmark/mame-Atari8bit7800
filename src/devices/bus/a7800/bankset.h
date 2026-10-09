// license:BSD-3-Clause
// copyright-holders:Mike Saarna
/***********************************************************************************************************

 Atari 7800 bankset carts, by Fred Quimby (batari)

 Ported from the a7800 fork (https://github.com/7800-devtools/a7800) to the current MAME device API.

 A bankset cart is two parallel sets of banks: Sally (the CPU) sees one half of the image and MARIA sees
 the other, at the same addresses. MARIA is the one reading while the machine is running a DMA cycle
 (set_dma_active). The SuperGame forms bank both halves with the CPU's one bank register, so MARIA's
 bank number is always the CPU's. With bank RAM, Sally sees 16K of RAM at $4000 and MARIA sees another
 16K there; Sally writes MARIA's RAM through $C000-$FFFF.

***********************************************************************************************************/

#ifndef MAME_BUS_A7800_BANKSET_H
#define MAME_BUS_A7800_BANKSET_H

#pragma once

#include "rom.h"
#include "sound/pokey.h"

// ======================> a78_bankset_sg_device

class a78_bankset_sg_device : public a78_rom_sg_device
{
public:
	a78_bankset_sg_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
};


// ======================> a78_bankset_sg_p800_device

class a78_bankset_sg_p800_device : public a78_rom_sg_device
{
public:
	a78_bankset_sg_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_bankset_sg_bankram_device

class a78_bankset_sg_bankram_device : public a78_rom_sg_device
{
public:
	a78_bankset_sg_bankram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
};


// ======================> a78_bankset_sg_bankram_p800_device

class a78_bankset_sg_bankram_p800_device : public a78_rom_sg_device
{
public:
	a78_bankset_sg_bankram_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_bankset_rom_device

class a78_bankset_rom_device : public a78_rom_device
{
public:
	a78_bankset_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;

protected:
};


// ======================> a78_bankset_rom_p800_device

class a78_bankset_rom_p800_device : public a78_rom_device
{
public:
	a78_bankset_rom_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_bankset_rom_p4000_device

class a78_bankset_rom_p4000_device : public a78_rom_device
{
public:
	a78_bankset_rom_p4000_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey4000;
};


// ======================> a78_bankset_bankram_device

class a78_bankset_bankram_device : public a78_rom_device
{
public:
	a78_bankset_bankram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
};


// ======================> a78_bankset_bankram_p800_device

class a78_bankset_bankram_p800_device : public a78_rom_device
{
public:
	a78_bankset_bankram_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_bankset_rom_52k_device

class a78_bankset_rom_52k_device : public a78_rom_device
{
public:
	a78_bankset_rom_52k_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual uint8_t read_30xx(offs_t offset) override;

protected:
};


// ======================> a78_bankset_rom_52k_p4000_device

class a78_bankset_rom_52k_p4000_device : public a78_rom_device
{
public:
	a78_bankset_rom_52k_p4000_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual uint8_t read_30xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey4000;
};


// device type definitions
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_SG, a78_bankset_sg_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_SG_POK800, a78_bankset_sg_p800_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_SG_BANKRAM, a78_bankset_sg_bankram_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_SG_BANKRAM_POK800, a78_bankset_sg_bankram_p800_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_ROM, a78_bankset_rom_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_POK800, a78_bankset_rom_p800_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_POK4000, a78_bankset_rom_p4000_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_BANKRAM, a78_bankset_bankram_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_BANKRAM_POK800, a78_bankset_bankram_p800_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_52K, a78_bankset_rom_52k_device)
DECLARE_DEVICE_TYPE(A78_ROM_BANKSET_ROM_52K_POK4000, a78_bankset_rom_52k_p4000_device)

#endif // MAME_BUS_A7800_BANKSET_H
