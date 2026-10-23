// license:BSD-3-Clause
// copyright-holders:Miasmark
/**********************************************************************

    Atari XG-1 light gun

    The light gun of the XE Game System, also sold for the 2600 (Sentinel,
    Shooting Arcade) and the 7800 (Alien Brigade, Barnyard Blaster,
    Crossbow, Meltdown, Sentinel).

    Pin 1 (Up): trigger. The gun holds it low while the trigger is
    released and lets it go high while it is pulled, the opposite of a
    joystick: 7800 Crossbow tells the gun from a joystick by Up reading
    low at power on, and 7800 Sentinel fires when it goes high.
    Pin 6 (Button): light sensor, low while the photodiode sees light,
    i.e. while the beam is drawing something bright where the gun points.
    Games poll it all along the scanlines (7800 Sentinel reads it every 5
    cycles) to find where the beam was when it went low.
    Pins 7 and 8: power for the sensor's amplifier.

    The sensing itself is left to the machine (see
    vcs_control_port_device::set_light_sense_callback), which knows where
    its video chip's beam is and what it has drawn.

    TODO:
    - the sensor's field of view, its response time and the brightness
      it needs (tia_video_device::light_sensor) are estimates, not
      measurements

**********************************************************************/

#include "emu.h"
#include "xg1.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(ATARI_XG1, atari_xg1_device, "atari_xg1", "Atari XG-1 Light Gun")


static INPUT_PORTS_START( atari_xg1 )
	PORT_START("TRIGGER")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("LIGHTX")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(45) PORT_KEYDELTA(15)

	PORT_START("LIGHTY")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(45) PORT_KEYDELTA(15)
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor atari_xg1_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( atari_xg1 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  atari_xg1_device - constructor
//-------------------------------------------------

atari_xg1_device::atari_xg1_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, ATARI_XG1, tag, owner, clock),
	device_vcs_control_port_interface(mconfig, *this),
	m_trigger(*this, "TRIGGER"),
	m_lightx(*this, "LIGHTX"),
	m_lighty(*this, "LIGHTY")
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void atari_xg1_device::device_start()
{
}


//-------------------------------------------------
//  vcs_joy_r - read the joystick pins
//-------------------------------------------------

uint8_t atari_xg1_device::vcs_joy_r()
{
	uint8_t data = 0xff;

	// pin 1: high only while the trigger is pulled
	if (!BIT(m_trigger->read(), 0))
		data &= ~0x01;

	// pin 6: low while the sensor sees light
	if (light_detected(m_lightx->read(), m_lighty->read()))
		data &= ~0x20;

	return data;
}
