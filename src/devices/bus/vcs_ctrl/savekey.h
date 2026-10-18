// license:BSD-3-Clause
// copyright-holders:a7800 toolkit contributors
/**********************************************************************

    SaveKey (and the memory of the AtariVox)

**********************************************************************/

#ifndef MAME_BUS_VCS_CTRL_SAVEKEY_H
#define MAME_BUS_VCS_CTRL_SAVEKEY_H

#pragma once

#include "ctrl.h"

#include "machine/i2cmem.h"


// ======================> vcs_savekey_device

class vcs_savekey_device : public device_t,
							public device_vcs_control_port_interface
{
public:
	// construction/destruction
	vcs_savekey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// device_vcs_control_port_interface overrides
	virtual uint8_t vcs_joy_r() override;
	virtual void vcs_joy_w(uint8_t data) override;

protected:
	// device-level overrides
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

private:
	required_device<i2cmem_device> m_eeprom;

	int m_scl;
	int m_sda;
};


// device type definition
DECLARE_DEVICE_TYPE(VCS_SAVEKEY, vcs_savekey_device)

#endif // MAME_BUS_VCS_CTRL_SAVEKEY_H
