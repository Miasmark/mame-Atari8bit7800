// license:BSD-3-Clause
// copyright-holders:Miasmark
/**********************************************************************

    Atari XG-1 light gun

**********************************************************************/

#ifndef MAME_BUS_VCS_CTRL_XG1_H
#define MAME_BUS_VCS_CTRL_XG1_H

#pragma once

#include "ctrl.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> atari_xg1_device

class atari_xg1_device : public device_t,
						public device_vcs_control_port_interface
{
public:
	// construction/destruction
	atari_xg1_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// device_vcs_control_port_interface overrides
	virtual uint8_t vcs_joy_r() override;

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;

private:
	required_ioport m_trigger;
	required_ioport m_lightx;
	required_ioport m_lighty;
};


// device type definition
DECLARE_DEVICE_TYPE(ATARI_XG1, atari_xg1_device)


#endif // MAME_BUS_VCS_CTRL_XG1_H
