// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
#ifndef MAME_BUS_VCS_SCHARGER_H
#define MAME_BUS_VCS_SCHARGER_H

#pragma once

#include "rom.h"
#include "imagedev/cassette.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> a26_rom_ss_device

class a26_rom_ss_device : public a26_rom_base_device
{
public:
	a26_rom_ss_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	virtual void install_memory_handlers(address_space *space) override;
	uint8_t read_lo(offs_t offset);
	uint8_t read_hi(offs_t offset);
	uint8_t read_cass(offs_t offset);

private:
	static constexpr uint32_t RAM_WRITE_TIME = 5;

	// tape images: 8K of data and a 256 byte header for each load
	static constexpr uint32_t TAPE_LOAD_SIZE = 0x2100;
	// offsets in the loader that stands in for the BIOS ROM
	static constexpr offs_t LOADER_START_LO = 0x024;
	static constexpr offs_t LOADER_START_HI = 0x028;
	static constexpr offs_t LOADER_CONTROL_RAM = 0x02c;
	static constexpr offs_t LOADER_CONTROL = 0x030;
	static constexpr offs_t LOADER_LOAD_REQUESTED = 0x7f0;
	static constexpr offs_t LOADER_LOAD_FIRST = 0x7f1;

	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	void install_tap_handler();

	required_device<cassette_image_device> m_cassette;

	void tap(offs_t offset);

	int m_base_banks[2];
	uint8_t m_data;
	uint8_t m_write_delay;
	bool m_ram_write_enabled;
	bool m_rom_enabled;
	uint16_t m_last_address_bus;
	uint32_t m_address_bus_changes;

	void setup_tape_loader();
	void tape_load(bool first);

	address_space *m_space;
	uint32_t m_tape_loads;
	uint8_t m_loader[0x800];
	uint32_t m_current_load;
};


// device type definition
DECLARE_DEVICE_TYPE(A26_ROM_SUPERCHARGER, a26_rom_ss_device)

#endif // MAME_BUS_VCS_SCHARGER_H
