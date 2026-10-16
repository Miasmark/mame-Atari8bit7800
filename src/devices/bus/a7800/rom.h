// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
#ifndef MAME_BUS_A7800_ROM_H
#define MAME_BUS_A7800_ROM_H

#pragma once

#include "a78_slot.h"
#include "sound/pokey.h"


// ======================> a78_rom_device

class a78_rom_device : public device_t,
						public device_a78_cart_interface
{
public:
	// construction/destruction
	a78_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;

protected:
	a78_rom_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
};


// ======================> a78_rom_pokey_device

class a78_rom_pokey_device : public a78_rom_device
{
public:
	// construction/destruction
	a78_rom_pokey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	a78_rom_pokey_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey;
};


// ======================> a78_rom_sg_ram_device

class a78_rom_mram_device : public a78_rom_device
{
public:
	// construction/destruction
	a78_rom_mram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	a78_rom_mram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);
};


// ======================> a78_rom_sg_device

class a78_rom_sg_device : public a78_rom_device
{
public:
	// construction/destruction
	a78_rom_sg_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	a78_rom_sg_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	int m_bank;
};


// ======================> a78_rom_sg_pokey_device

class a78_rom_sg_pokey_device : public a78_rom_sg_device
{
public:
	// construction/destruction
	a78_rom_sg_pokey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey;
};


// ======================> a78_rom_sg_ram_device

class a78_rom_sg_ram_device : public a78_rom_sg_device
{
public:
	// construction/destruction
	a78_rom_sg_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	a78_rom_sg_ram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);
};


// ======================> a78_rom_sg9_device

class a78_rom_sg9_device : public a78_rom_sg_device
{
public:
	// construction/destruction
	a78_rom_sg9_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	a78_rom_sg9_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);
};


// ======================> a78_rom_abs_device

class a78_rom_abs_device : public a78_rom_device
{
public:
	// construction/destruction
	a78_rom_abs_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	int m_bank;
};


// ======================> a78_rom_act_device

class a78_rom_act_device : public a78_rom_device
{
public:
	// construction/destruction
	a78_rom_act_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_40xx(offs_t offset) override;
	virtual void write_40xx(offs_t offset, uint8_t data) override;

protected:
	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	int m_bank;
};


// PCB variants with a POKEY at $0450

// ======================> a78_rom_p450_device

class a78_rom_p450_device : public a78_rom_device
{
public:
	// construction/destruction
	a78_rom_p450_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_04xx(offs_t offset) override { if (offset >= 0x50 && offset < 0x60) return m_pokey450->read(offset & 0x0f); else return 0xff; }
	virtual void write_04xx(offs_t offset, uint8_t data) override { if (offset >= 0x50 && offset < 0x60) m_pokey450->write(offset & 0x0f, data); }

protected:
	a78_rom_p450_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey450;
};


// ======================> a78_rom_p450_pokey_device

class a78_rom_p450_pokey_device : public a78_rom_pokey_device
{
public:
	// construction/destruction
	a78_rom_p450_pokey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_04xx(offs_t offset) override { if (offset >= 0x50 && offset < 0x60) return m_pokey450->read(offset & 0x0f); else return 0xff; }
	virtual void write_04xx(offs_t offset, uint8_t data) override { if (offset >= 0x50 && offset < 0x60) m_pokey450->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey450;
};


// ======================> a78_rom_p450_sg_ram_device

class a78_rom_p450_sg_ram_device : public a78_rom_sg_ram_device
{
public:
	// construction/destruction
	a78_rom_p450_sg_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_04xx(offs_t offset) override { if (offset >= 0x50 && offset < 0x60) return m_pokey450->read(offset & 0x0f); else return 0xff; }
	virtual void write_04xx(offs_t offset, uint8_t data) override { if (offset >= 0x50 && offset < 0x60) m_pokey450->write(offset & 0x0f, data); }

protected:
	a78_rom_p450_sg_ram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey450;
};


// ======================> a78_rom_p450_sg9_device

class a78_rom_p450_sg9_device : public a78_rom_sg9_device
{
public:
	// construction/destruction
	a78_rom_p450_sg9_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_04xx(offs_t offset) override { if (offset >= 0x50 && offset < 0x60) return m_pokey450->read(offset & 0x0f); else return 0xff; }
	virtual void write_04xx(offs_t offset, uint8_t data) override { if (offset >= 0x50 && offset < 0x60) m_pokey450->write(offset & 0x0f, data); }

protected:
	a78_rom_p450_sg9_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey450;
};


// ======================> a78_pokey440_device

// A $0450 POKEY cart with a second POKEY at $0440 (the "Dual POKEY 440 450" homebrew boards:
// header bit $0400, A78 v4 audio field 3). The two are separate chips, each with its own AUDCTL,
// timers and RANDOM, mixed on the cart's audio line.
template <class Base>
class a78_pokey440_device : public Base
{
public:
	virtual uint8_t read_04xx(offs_t offset) override
	{
		if (offset >= 0x40 && offset < 0x50)
			return m_pokey440->read(offset & 0x0f);
		return Base::read_04xx(offset);
	}
	virtual void write_04xx(offs_t offset, uint8_t data) override
	{
		if (offset >= 0x40 && offset < 0x50)
			m_pokey440->write(offset & 0x0f, data);
		else
			Base::write_04xx(offset, data);
	}

protected:
	a78_pokey440_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock, const char *speaker)
		: Base(mconfig, type, tag, owner, clock)
		, m_pokey440(*this, "pokey440")
		, m_speaker(speaker)
	{
	}

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD
	{
		// each chip at half, as MAME's two-POKEY machines are, so the two at full volume fit
		Base::device_add_mconfig(config);
		Base::m_pokey450.lookup()->reset_routes().add_route(ALL_OUTPUTS, m_speaker, 0.50);
		POKEY(config, m_pokey440, DERIVED_CLOCK(1, 1)).add_route(ALL_OUTPUTS, m_speaker, 0.50);
	}

	required_device<pokey_device> m_pokey440;
	const char *const m_speaker;
};

class a78_rom_p440_p450_device : public a78_pokey440_device<a78_rom_p450_device>
{
public:
	a78_rom_p440_p450_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
};

class a78_rom_p440_p450_sg_ram_device : public a78_pokey440_device<a78_rom_p450_sg_ram_device>
{
public:
	a78_rom_p440_p450_sg_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
};

class a78_rom_p440_p450_sg9_device : public a78_pokey440_device<a78_rom_p450_sg9_device>
{
public:
	a78_rom_p440_p450_sg9_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
};






// PCB variants with a POKEY at $0800 (homebrew pcb)

// ======================> a78_rom_p800_device

class a78_rom_p800_device : public a78_rom_device
{
public:
	// construction/destruction
	a78_rom_p800_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_rom_p800_pokey_device

class a78_rom_p800_pokey_device : public a78_rom_pokey_device
{
public:
	// construction/destruction
	a78_rom_p800_pokey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_rom_p800_sg_device

class a78_rom_p800_sg_device : public a78_rom_sg_device
{
public:
	// construction/destruction
	a78_rom_p800_sg_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_rom_p800_sg_ram_device

class a78_rom_p800_sg_ram_device : public a78_rom_sg_ram_device
{
public:
	// construction/destruction
	a78_rom_p800_sg_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// ======================> a78_rom_p800_sg9_device

class a78_rom_p800_sg9_device : public a78_rom_sg9_device
{
public:
	// construction/destruction
	a78_rom_p800_sg9_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// reading and writing
	virtual uint8_t read_08xx(offs_t offset) override { return m_pokey800->read(offset & 0x0f); }
	virtual void write_08xx(offs_t offset, uint8_t data) override { m_pokey800->write(offset & 0x0f, data); }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<pokey_device> m_pokey800;
};


// device type definition
DECLARE_DEVICE_TYPE(A78_ROM,             a78_rom_device)
DECLARE_DEVICE_TYPE(A78_ROM_SG,          a78_rom_sg_device)
DECLARE_DEVICE_TYPE(A78_ROM_POKEY,       a78_rom_pokey_device)
DECLARE_DEVICE_TYPE(A78_ROM_SG_POKEY,    a78_rom_sg_pokey_device)
DECLARE_DEVICE_TYPE(A78_ROM_SG_RAM,      a78_rom_sg_ram_device)
DECLARE_DEVICE_TYPE(A78_ROM_MRAM,        a78_rom_mram_device)
DECLARE_DEVICE_TYPE(A78_ROM_SG9,         a78_rom_sg9_device)
DECLARE_DEVICE_TYPE(A78_ROM_ABSOLUTE,    a78_rom_abs_device)
DECLARE_DEVICE_TYPE(A78_ROM_ACTIVISION,  a78_rom_act_device)

// PCB variants with a POKEY at $0450
DECLARE_DEVICE_TYPE(A78_ROM_P450,        a78_rom_p450_device)
DECLARE_DEVICE_TYPE(A78_ROM_P450_POKEY,  a78_rom_p450_pokey_device)
DECLARE_DEVICE_TYPE(A78_ROM_P450_SG_RAM, a78_rom_p450_sg_ram_device)
DECLARE_DEVICE_TYPE(A78_ROM_P450_SG9,    a78_rom_p450_sg9_device)
DECLARE_DEVICE_TYPE(A78_ROM_P440_P450,        a78_rom_p440_p450_device)
DECLARE_DEVICE_TYPE(A78_ROM_P440_P450_SG_RAM, a78_rom_p440_p450_sg_ram_device)
DECLARE_DEVICE_TYPE(A78_ROM_P440_P450_SG9,    a78_rom_p440_p450_sg9_device)

// PCB variants with a POKEY at $0800
DECLARE_DEVICE_TYPE(A78_ROM_P800,        a78_rom_p800_device)
DECLARE_DEVICE_TYPE(A78_ROM_P800_POKEY,  a78_rom_p800_pokey_device)
DECLARE_DEVICE_TYPE(A78_ROM_P800_SG,     a78_rom_p800_sg_device)
DECLARE_DEVICE_TYPE(A78_ROM_P800_SG_RAM, a78_rom_p800_sg_ram_device)
DECLARE_DEVICE_TYPE(A78_ROM_P800_SG9,    a78_rom_p800_sg9_device)

#endif // MAME_BUS_A7800_ROM_H
