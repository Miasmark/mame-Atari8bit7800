// license:BSD-3-Clause
// copyright-holders:a7800 toolkit contributors
/**********************************************************************

    SaveKey (and the memory of the AtariVox)

    A 32K I2C EEPROM (24LC256) on the joystick pins of a controller port,
    usually the right one: pin 3 (Left) carries SDA and pin 4 (Right) SCL.
    Both lines are open drain, pulled up: the console takes a line low by
    making its RIOT pin an output at 0, and lets it go by making the pin
    an input again, when the EEPROM can pull SDA low to answer.

    The AtariVox has the same memory, and a speech chip on pins 1 and 2:
    see atarivox.cpp.

**********************************************************************/

#include "emu.h"
#include "savekey.h"


DEFINE_DEVICE_TYPE(VCS_SAVEKEY, vcs_savekey_device, "vcs_savekey", "SaveKey")


vcs_savekey_device::vcs_savekey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	vcs_savekey_device(mconfig, VCS_SAVEKEY, tag, owner, clock)
{
}

vcs_savekey_device::vcs_savekey_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_vcs_control_port_interface(mconfig, *this),
	m_eeprom(*this, "eeprom"),
	m_scl(1),
	m_sda(1)
{
}

void vcs_savekey_device::device_add_mconfig(machine_config &config)
{
	I2C_24C256(config, m_eeprom);
}

void vcs_savekey_device::device_start()
{
	save_item(NAME(m_scl));
	save_item(NAME(m_sda));
}


void vcs_savekey_device::device_reset()
{
	// the console lets the lines go on reset; the EEPROM has to see them high before a start
	m_scl = m_sda = 1;
	m_eeprom->write_sda(1);
	m_eeprom->write_scl(1);
}


// the lines as the console drives them: a pin that is an input reads as 1 here, and lets the line go
void vcs_savekey_device::vcs_joy_w(uint8_t data)
{
	int const scl = BIT(data, 3);
	int const sda = BIT(data, 2);

	// data changes while the clock is low; a change of SDA with the clock high is a start or a stop
	if (scl != m_scl && !scl)
		m_eeprom->write_scl(0);
	if (sda != m_sda)
		m_eeprom->write_sda(sda);
	if (scl != m_scl && scl)
		m_eeprom->write_scl(1);

	m_scl = scl;
	m_sda = sda;
}

uint8_t vcs_savekey_device::vcs_joy_r()
{
	// SDA is low when the console or the EEPROM holds it low
	return (m_sda && m_eeprom->read_sda()) ? 0xff : ~0x04;
}
