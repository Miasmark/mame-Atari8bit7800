// license:BSD-3-Clause
// copyright-holders:a7800 toolkit contributors
#include "emu.h"
#include "vcs_carts.h"

#include "compumat.h"
#include "dpc.h"
#include "harmony_melody.h"
#include "rom.h"
#include "scharger.h"


void vcs_cart(device_slot_interface &device)
{
	device.option_add("a26_2k_4k", A26_ROM_2K_4K);
	device.option_add("a26_f4",    A26_ROM_F4);
	device.option_add("a26_f6",    A26_ROM_F6);
	device.option_add("a26_f8",    A26_ROM_F8);
	device.option_add("a26_f8sw",  A26_ROM_F8_SW);
	device.option_add("a26_ef",    A26_ROM_EF);
	device.option_add("a26_df",    A26_ROM_DF);
	device.option_add("a26_bf",    A26_ROM_BF);
	device.option_add("a26_sb",    A26_ROM_SB);
	device.option_add("a26_mdm",   A26_ROM_MDM);
	device.option_add("a26_fa",    A26_ROM_FA);
	device.option_add("a26_fa2",   A26_ROM_FA2);
	device.option_add("a26_fe",    A26_ROM_FE);
	device.option_add("a26_3e",    A26_ROM_3E);
	device.option_add("a26_3f",    A26_ROM_3F);
	device.option_add("a26_3ep",   A26_ROM_3EP);
	device.option_add("a26_e0",    A26_ROM_E0);
	device.option_add("a26_e7",    A26_ROM_E7);
	device.option_add("a26_ua",    A26_ROM_UA);
	device.option_add("a26_uasw",  A26_ROM_UASW);
	device.option_add("a26_0840",  A26_ROM_0840);
	device.option_add("a26_0fa0",  A26_ROM_0FA0);
	device.option_add("a26_03e0",  A26_ROM_03E0);
	device.option_add("a26_cv",    A26_ROM_CV);
	device.option_add("a26_dc",    A26_ROM_DC);
	device.option_add("a26_fv",    A26_ROM_FV);
	device.option_add("a26_jvp",   A26_ROM_JVP);
	device.option_add("a26_cm",    A26_ROM_COMPUMATE);
	device.option_add("a26_ss",    A26_ROM_SUPERCHARGER);
	device.option_add("a26_dpc",   A26_ROM_DPC);
	device.option_add("a26_4in1",  A26_ROM_4IN1);
	device.option_add("a26_8in1",  A26_ROM_8IN1);
	device.option_add("a26_32in1", A26_ROM_32IN1);
	device.option_add("a26_x07",   A26_ROM_X07);
	device.option_add("a26_harmony",   A26_ROM_HARMONY);
	device.option_add("a26_f0",    A26_ROM_F0);
}
