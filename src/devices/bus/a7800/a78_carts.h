// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
#ifndef MAME_BUS_A7800_A78_CARTS_H
#define MAME_BUS_A7800_A78_CARTS_H

#pragma once

#include "rom.h"
#include "xboard.h"
#include "hiscore.h"
#include "cpuwiz.h"
#include "bankset.h"

static void a7800_cart(device_slot_interface &device)
{
	device.option_add_internal("a78_rom",      A78_ROM);
	device.option_add_internal("a78_pokey",    A78_ROM_POKEY);
	device.option_add_internal("a78_sg",       A78_ROM_SG);
	device.option_add_internal("a78_sg_pokey", A78_ROM_SG_POKEY);
	device.option_add_internal("a78_sg_ram",   A78_ROM_SG_RAM);
	device.option_add_internal("a78_sg9",      A78_ROM_SG9);
	device.option_add_internal("a78_mram",     A78_ROM_MRAM);
	device.option_add_internal("a78_abs",      A78_ROM_ABSOLUTE);
	device.option_add_internal("a78_act",      A78_ROM_ACTIVISION);
	device.option_add_internal("a78_hsc",      A78_HISCORE);
	device.option_add_internal("a78_xboard",   A78_XBOARD); // the actual XBoarD expansion (as passthru)
	device.option_add_internal("a78_xm",       A78_XM);     // the actual XM expansion (as passthru)
	device.option_add_internal("a78_megacart", A78_ROM_MEGACART);
	device.option_add_internal("a78_versa",    A78_ROM_VERSABOARD);
	// cart variants with a POKEY at 0x0450 (typically a VersaBoard variant, or an homebrew pcb)
	device.option_add_internal("a78_p450_t0",  A78_ROM_P450);
	device.option_add_internal("a78_p450_t1",  A78_ROM_P450_POKEY);
	device.option_add_internal("a78_p450_t6",  A78_ROM_P450_SG_RAM);
	device.option_add_internal("a78_p450_ta",  A78_ROM_P450_SG9);
	device.option_add_internal("a78_p450_vb",  A78_ROM_P450_VB);
	device.option_add_internal("a78_p440_t0",  A78_ROM_P440_P450);
	device.option_add_internal("a78_p440_t6",  A78_ROM_P440_P450_SG_RAM);
	device.option_add_internal("a78_p440_ta",  A78_ROM_P440_P450_SG9);
	device.option_add_internal("a78_p440_vb",  A78_ROM_P440_P450_VB);
	// cart variants with a POKEY at 0x0800 (homebrew pcb)
	device.option_add_internal("a78_p800_t0",  A78_ROM_P800);
	device.option_add_internal("a78_p800_t1",  A78_ROM_P800_POKEY);
	device.option_add_internal("a78_p800_t2",  A78_ROM_P800_SG);
	device.option_add_internal("a78_p800_t6",  A78_ROM_P800_SG_RAM);
	device.option_add_internal("a78_p800_ta",  A78_ROM_P800_SG9);
	// bankset carts: two parallel sets of banks, one for the CPU and one for MARIA
	device.option_add_internal("a78_bankset",                    A78_ROM_BANKSET_ROM);
	device.option_add_internal("a78_bankset_p800",               A78_ROM_BANKSET_ROM_POK800);
	device.option_add_internal("a78_bankset_p4000",              A78_ROM_BANKSET_ROM_POK4000);
	device.option_add_internal("a78_bankset_bankram",            A78_ROM_BANKSET_BANKRAM);
	device.option_add_internal("a78_bankset_bankram_p800",       A78_ROM_BANKSET_BANKRAM_POK800);
	device.option_add_internal("a78_bankset_52k",                A78_ROM_BANKSET_ROM_52K);
	device.option_add_internal("a78_bankset_52k_p4000",          A78_ROM_BANKSET_ROM_52K_POK4000);
	device.option_add_internal("a78_bankset_sg",                 A78_ROM_BANKSET_SG);
	device.option_add_internal("a78_bankset_sg_p800",            A78_ROM_BANKSET_SG_POK800);
	device.option_add_internal("a78_bankset_sg_bankram",         A78_ROM_BANKSET_SG_BANKRAM);
	device.option_add_internal("a78_bankset_sg_ram",             A78_ROM_BANKSET_SG_RAM);
	device.option_add_internal("a78_bankset_sg_ram_p800",        A78_ROM_BANKSET_SG_RAM_POK800);
	device.option_add_internal("a78_bankset_sg_bankram_p800",    A78_ROM_BANKSET_SG_BANKRAM_POK800);
}


// supported devices
void a78_cart(device_slot_interface &device);

#endif // MAME_BUS_A7800_A78_CARTS_H
