// license:BSD-3-Clause
// copyright-holders:a7800 toolkit contributors
#include "emu.h"
#include "vcs_carts.h"

#include "compumat.h"
#include "dpc.h"
#include "harmony_melody.h"
#include "rom.h"
#include "scharger.h"


template <typename T>
static void vcs_cart_list(T &&add)
{
	add("a26_2k_4k",   A26_ROM_2K_4K);
	add("a26_f4",      A26_ROM_F4);
	add("a26_f6",      A26_ROM_F6);
	add("a26_f8",      A26_ROM_F8);
	add("a26_f8sw",    A26_ROM_F8_SW);
	add("a26_ef",      A26_ROM_EF);
	add("a26_df",      A26_ROM_DF);
	add("a26_bf",      A26_ROM_BF);
	add("a26_sb",      A26_ROM_SB);
	add("a26_mdm",     A26_ROM_MDM);
	add("a26_fa",      A26_ROM_FA);
	add("a26_fa2",     A26_ROM_FA2);
	add("a26_fe",      A26_ROM_FE);
	add("a26_3e",      A26_ROM_3E);
	add("a26_3f",      A26_ROM_3F);
	add("a26_3ep",     A26_ROM_3EP);
	add("a26_e0",      A26_ROM_E0);
	add("a26_e7",      A26_ROM_E7);
	add("a26_ua",      A26_ROM_UA);
	add("a26_uasw",    A26_ROM_UASW);
	add("a26_0840",    A26_ROM_0840);
	add("a26_0fa0",    A26_ROM_0FA0);
	add("a26_03e0",    A26_ROM_03E0);
	add("a26_cv",      A26_ROM_CV);
	add("a26_dc",      A26_ROM_DC);
	add("a26_fv",      A26_ROM_FV);
	add("a26_jvp",     A26_ROM_JVP);
	add("a26_cm",      A26_ROM_COMPUMATE);
	add("a26_ss",      A26_ROM_SUPERCHARGER);
	add("a26_dpc",     A26_ROM_DPC);
	add("a26_4in1",    A26_ROM_4IN1);
	add("a26_8in1",    A26_ROM_8IN1);
	add("a26_32in1",   A26_ROM_32IN1);
	add("a26_x07",     A26_ROM_X07);
	add("a26_harmony", A26_ROM_HARMONY);
	add("a26_f0",      A26_ROM_F0);
}

void vcs_cart(device_slot_interface &device)
{
	vcs_cart_list([&device] (char const *name, auto const &type) { device.option_add(name, type); });
}

void vcs_cart_internal(device_slot_interface &device)
{
	vcs_cart_list([&device] (char const *name, auto const &type) { device.option_add_internal(name, type); });
}
