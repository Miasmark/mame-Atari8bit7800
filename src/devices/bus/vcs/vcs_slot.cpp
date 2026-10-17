// license:BSD-3-Clause
// copyright-holders:Fabio Priuli,Cowering
/***********************************************************************************************************


    Atari VCS 2600 cart emulation
    (through slot devices)

    Emulation of the cartslot for Atari 2600
    Several banking schemes have been used for larger roms,
    and some carts contained RAM (so called "Special Chip")

    Mapper identification routines based on Cowering's code.

 ***********************************************************************************************************/

#include "emu.h"
#include "vcs_slot.h"

#include <algorithm>
#include <string_view>

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(VCS_CART_SLOT, vcs_cart_slot_device, "vcs_cart_slot", "Atari VCS 2600 Cartridge Slot")

/* PCB */
enum
{
	A26_2K_4K = 0,
	A26_F4,
	A26_F6,
	A26_F8,
	A26_F8SW,
	A26_FA,
	A26_FE,
	A26_3E,     // to test
	A26_3F,
	A26_E0,
	A26_E7,
	A26_UA,
	A26_DC,
	A26_CV,
	A26_FV,
	A26_JVP,    // to test
	A26_32IN1,
	A26_8IN1,
	A26_4IN1,
	A26_DPC,
	A26_SS,
	A26_CM,
	A26_X07,
	A26_HARMONY,
	A26_F0,
	A26_FA2,
	A26_EF,
	A26_DF,
	A26_BF,
	A26_SB,
	A26_UASW,
	A26_0840,
	A26_0FA0,
	A26_03E0,
	A26_3EP,
	A26_MDM,
};

//-------------------------------------------------
//  device_vcs_cart_interface - constructor
//-------------------------------------------------

device_vcs_cart_interface::device_vcs_cart_interface(const machine_config &mconfig, device_t &device) :
	device_interface(device, "vcscart"),
	m_rom(nullptr),
	m_rom_size(0)
{
}


//-------------------------------------------------
//  ~device_vcs_cart_interface - destructor
//-------------------------------------------------

device_vcs_cart_interface::~device_vcs_cart_interface()
{
}

//-------------------------------------------------
//  rom_alloc - alloc the space for the cart
//-------------------------------------------------

void device_vcs_cart_interface::rom_alloc(uint32_t size, const char *tag)
{
	if (m_rom == nullptr)
	{
		m_rom = device().machine().memory().region_alloc(std::string(tag).append(":cart:rom"), size, 1, ENDIANNESS_LITTLE)->base();
		m_rom_size = size;
	}
}

//-------------------------------------------------
//  ram_alloc - alloc the space for the on-cart RAM
//-------------------------------------------------

void device_vcs_cart_interface::ram_alloc(uint32_t size)
{
	m_ram.resize(size);
	device().save_item(NAME(m_ram));
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vcs_cart_slot_device - constructor
//-------------------------------------------------
vcs_cart_slot_device::vcs_cart_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, VCS_CART_SLOT, tag, owner, clock),
	device_cartrom_image_interface(mconfig, *this),
	device_single_card_slot_interface<device_vcs_cart_interface>(mconfig, *this),
	m_cart(nullptr),
	m_type(0),
	m_address_space(*this, finder_base::DUMMY_TAG, -1, 8)
{
}


//-------------------------------------------------
//  vcs_cart_slot_device - destructor
//-------------------------------------------------

vcs_cart_slot_device::~vcs_cart_slot_device()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vcs_cart_slot_device::device_start()
{
	m_cart = get_card_device();
}



/*-------------------------------------------------
 call load
 -------------------------------------------------*/

//-------------------------------------------------
//  VCS PCBs
//-------------------------------------------------

struct vcs_slot
{
	int                     pcb_id;
	const char              *slot_option;
};

// Here, we take the feature attribute from .xml (i.e. the PCB name) and we assign a unique ID to it
static const vcs_slot slot_list[] =
{
	{ A26_2K_4K, "a26_2k_4k" },
	{ A26_F4, "a26_f4" },
	{ A26_F6, "a26_f6" },
	{ A26_F8, "a26_f8" },
	{ A26_F8SW, "a26_f8sw" },
	{ A26_FA, "a26_fa" },
	{ A26_FE, "a26_fe" },
	{ A26_E0, "a26_e0" },
	{ A26_E7, "a26_e7" },
	{ A26_3E, "a26_3e" },
	{ A26_3F, "a26_3f" },
	{ A26_UA, "a26_ua" },
	{ A26_CV, "a26_cv" },
	{ A26_DC, "a26_dc" },
	{ A26_FV, "a26_fv" },
	{ A26_JVP, "a26_jvp" },
	{ A26_CM, "a26_cm" },
	{ A26_SS, "a26_ss" },
	{ A26_DPC, "a26_dpc" },
	{ A26_4IN1, "a26_4in1" },
	{ A26_8IN1, "a26_8in1" },
	{ A26_32IN1, "a26_32in1" },
	{ A26_X07, "a26_x07" },
	{ A26_HARMONY, "a26_harmony" },
	{ A26_F0, "a26_f0" },
	{ A26_FA2, "a26_fa2" },
	{ A26_EF, "a26_ef" },
	{ A26_DF, "a26_df" },
	{ A26_BF, "a26_bf" },
	{ A26_SB, "a26_sb" },
	{ A26_UASW, "a26_uasw" },
	{ A26_0840, "a26_0840" },
	{ A26_0FA0, "a26_0fa0" },
	{ A26_03E0, "a26_03e0" },
	{ A26_3EP, "a26_3ep" },
	{ A26_MDM, "a26_mdm" },
};

static int vcs_get_pcb_id(const char *slot)
{
	for (auto & elem : slot_list)
	{
		if (!strcmp(elem.slot_option, slot))
			return elem.pcb_id;
	}

	return 0;
}

static const char *vcs_get_slot(int type)
{
	for (auto & elem : slot_list)
	{
		if (elem.pcb_id == type)
			return elem.slot_option;
	}

	return "a26_2k_4k";
}

std::pair<std::error_condition, std::string> vcs_cart_slot_device::call_load()
{
	if (m_cart)
	{
		const uint32_t len = loaded_through_softlist() ? get_software_region_length("rom") : length();

		// check that filesize is among the supported ones: besides the usual sizes, files can
		// be Supercharger tape images (8448 bytes for each load), and the 3E/3F/3E+ and MDM
		// boards take any number of banks
		bool const usual_size = is_usual_size(len);
		if (!usual_size && (loaded_through_softlist() || (!is_ss_tape_size(len) && !is_any_banks_size(len))))
			return std::make_pair(image_error::INVALIDLENGTH, "Unsupported cartridge ROM file size");

		m_cart->rom_alloc(len, tag());
		uint8_t *const ROM = m_cart->get_rom_base();

		if (loaded_through_softlist())
		{
			memcpy(ROM, get_software_region("rom"), len);
			const char *pcb_name = get_feature("slot");

			if (pcb_name != nullptr)
				m_type = vcs_get_pcb_id(pcb_name);
			else
			{
				// identify type based on size
				m_type = default_type_for_size(len);
				if (m_type < 0)
				{
					logerror("No slot type in the software list and no default for a %u byte cartridge, assuming 2K/4K\n", len);
					m_type = A26_2K_4K;
				}
			}

			if (get_software_region("ram"))
				m_cart->ram_alloc(get_software_region_length("ram"));
		}
		else
		{
			fread(ROM, len);

			// the board is the slot option picked by get_default_card_software(), unless
			// one was given on the command line: allocate the RAM that one needs
			m_type = vcs_get_pcb_id(m_cart->device().basetag());
			if (!usual_size && m_type != A26_SS && m_type != A26_3E && m_type != A26_3F && m_type != A26_3EP && m_type != A26_MDM)
				return std::make_pair(image_error::INVALIDLENGTH, "Unsupported cartridge ROM file size");

			// check for Super Chip (128bytes of RAM)
			if (len == 0x2000 || len == 0x4000 || len == 0x8000 || m_type == A26_EF || m_type == A26_DF || m_type == A26_BF)
				if (detect_super_chip(ROM, len))
				{
					m_cart->ram_alloc(0x80);
				}

			// add CBS RAM+ (256bytes of RAM)
			if (m_type == A26_FA || m_type == A26_FA2)
				m_cart->ram_alloc(0x100);
			// add M Network RAM
			else if (m_type == A26_E7)
				m_cart->ram_alloc(0x800);
			// add Commavid RAM
			else if (m_type == A26_CV)
				m_cart->ram_alloc(0x400);
			// add Starpath Superchager RAM
			else if (m_type == A26_SS)
				m_cart->ram_alloc(0x1800);
			// add Boulder Dash RAM, 3E+ RAM
			else if (m_type == A26_3E || m_type == A26_3EP)
				m_cart->ram_alloc(0x8000);
		}

		// pass a pointer to the now allocated ROM for the DPC chip
		if (m_type == A26_DPC)
			m_cart->setup_addon_ptr((uint8_t *)m_cart->get_rom_base() + 0x2000);

		m_cart->install_memory_handlers(m_address_space.target());
	}

	return std::make_pair(std::error_condition(), std::string());
}


/*-------------------------------------------------
 call_unload
 -------------------------------------------------*/

void vcs_cart_slot_device::call_unload()
{
}



/*-------------------------------------------------
  detection helper routines
 -------------------------------------------------*/

bool vcs_cart_slot_device::detect_modeDC(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	// signature is also in 'video reflex'.. maybe figure out that controller port someday...
	static const unsigned char signature[3] = { 0x8d, 0xf0, 0xff };

	if (len == 0x10000)
	{
		for (int i = 0; i < len - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature, sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeF6(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signature[3] = { 0x8d, 0xf6, 0xff };

	if (len == 0x4000)
	{
		for (int i = 0; i < len - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature, sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_snowhite(const uint8_t *cart, uint32_t len)
{
	static const unsigned char snowwhite[] = { 0x10, 0xd0, 0xff, 0xff }; // Snow White Proto

	if (len == 0x2000 && !memcmp(&cart[0x1ffc], snowwhite, sizeof(snowwhite)))
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_f8_bank1_start(const uint8_t *cart, uint32_t len)
{
	// MAME's F8 board starts in bank 0, which suits nearly every game, but on a few
	// (Congo Bongo) the reset vector of bank 0 points into game code that first reads
	// zero page RAM: this only works if the board comes up in bank 1, which those
	// games have an init routine for
	if (len != 0x2000)
		return 0;

	auto reads_ram_first = [cart] (uint32_t bank)
	{
		uint16_t const reset = (cart[bank + 0xffd] << 8) | cart[bank + 0xffc];
		uint8_t const *const code = &cart[bank + (reset & 0x0fff)];
		// LDA/LDX/LDY/ORA/AND/EOR/CMP zp with a RIOT RAM address
		bool const zp_read = code[0] == 0xa5 || code[0] == 0xa6 || code[0] == 0xa4 || code[0] == 0x05 || code[0] == 0x25 || code[0] == 0x45 || code[0] == 0xc5;
		return (reset & 0x1000) && ((reset & 0x0fff) < 0x0ffe) && zp_read && code[1] >= 0x80;
	};

	return reads_ram_first(0) && !reads_ram_first(0x1000);
}

bool vcs_cart_slot_device::detect_mode3E(const uint8_t *cart, uint32_t len)
{
	// this one is a little hacky... looks for STY $3e, which is unique to
	// 'not boulderdash', but is the only example I have (cow)
	// Would have used STA $3e, but 'Alien' and 'Star Raiders' do that for unknown reasons
	int numfound = 0;
	static const unsigned char signature[3] = { 0x84, 0x3e, 0x9d };

	if (len == 0x0800 || len == 0x1000)
	{
		for (int i = 0; i < len - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature, sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeSS(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signature[5] = { 0xbd, 0xe5, 0xff, 0x95, 0x81 };

	if (len == 0x0800 || len == 0x1000)
	{
		for (int i = 0; i < len - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature, sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::is_usual_size(uint32_t len)
{
	switch (len)
	{
		case 0x00800:
		case 0x01000:
		case 0x02000:
		case 0x028ff:
		case 0x02900:
		case 0x03000:
		case 0x04000:
		case 0x06000:
		case 0x07000:
		case 0x07400:
		case 0x08000:
		case 0x10000:
		case 0x20000:
		case 0x40000:
		case 0x80000:
			return true;

		default:
			return false;
	}
}

bool vcs_cart_slot_device::is_any_banks_size(uint32_t len)
{
	return len && !(len % 0x400) && len <= 0x80000;
}

int vcs_cart_slot_device::count_zp_stores(const uint8_t *cart, uint32_t len, uint8_t address)
{
	// STA, STX, STY zero page
	int found = 0;
	for (uint32_t i = 0; i + 1 < len; i++)
		if ((cart[i] == 0x85 || cart[i] == 0x86 || cart[i] == 0x84) && cart[i + 1] == address)
			found++;
	return found;
}

bool vcs_cart_slot_device::detect_mode3EP(const uint8_t *cart, uint32_t len)
{
	// up to 64K of 1K banks
	if (len < 0x800 || len > 0x10000)
		return 0;

	// 3E+ images are meant to carry a "TJ3E" marker
	static const char marker[] = { 'T', 'J', '3', 'E' };
	if (std::search(cart, cart + len, std::begin(marker), std::end(marker)) != cart + len)
		return 1;

	// otherwise: the reset vector is at the end of the first 1K, pointing to the last segment
	// (where that bank starts), and banks are put into segments other than the first
	// (LDA #$40-$FF / STA $3E or $3F), which 3E and 3F bank numbers (up to 64K) never need
	uint16_t const reset = (cart[0x3fd] << 8) | cart[0x3fc];
	int segments = 0;
	for (uint32_t i = 0; i + 3 < len; i++)
		if (cart[i] == 0xa9 && cart[i + 1] >= 0x40 && cart[i + 2] == 0x85 && (cart[i + 3] == 0x3e || cart[i + 3] == 0x3f))
			segments++;
	return ((reset & 0x1c00) == 0x1c00) && segments >= 2;
}

bool vcs_cart_slot_device::detect_any_3E(const uint8_t *cart, uint32_t len)
{
	// RAM and ROM banks are both selected
	return len >= 0x2000 && count_zp_stores(cart, len, 0x3e) >= 2 && count_zp_stores(cart, len, 0x3f) >= 1;
}

bool vcs_cart_slot_device::detect_any_3F(const uint8_t *cart, uint32_t len)
{
	// ROM banks are selected, RAM ones are not
	return len >= 0x2000 && count_zp_stores(cart, len, 0x3f) >= 3 && !count_zp_stores(cart, len, 0x3e);
}

bool vcs_cart_slot_device::is_ss_tape_size(uint32_t len)
{
	return len && !(len % 0x2100) && (len / 0x2100) <= 0x40;
}

bool vcs_cart_slot_device::detect_ss_tape(const uint8_t *cart, uint32_t len)
{
	// each load's 8 byte header (at 0x2000 in its record) sums up to 0x55
	if (!is_ss_tape_size(len))
		return 0;

	for (uint32_t load = 0; load < len; load += 0x2100)
	{
		uint8_t sum = 0;
		for (int i = 0; i < 8; i++)
			sum += cart[load + 0x2000 + i];
		if (sum != 0x55)
			return 0;
	}
	return 1;
}

bool vcs_cart_slot_device::detect_modeFA2(const uint8_t *cart, uint32_t len)
{
	// 32K FA2 images are the Harmony's 1K ARM driver, 28K of banks and 3K of padding
	if (len != 0x8000 || !has_arm_driver(cart, len))
		return 0;

	for (uint32_t i = 0x7401; i < len; i++)
		if (cart[i] != cart[0x7400])
			return 0;

	return count_abs_accesses(cart, len, 0x1ff5, 0x1ffb) >= 2;
}

bool vcs_cart_slot_device::has_arm_driver(const uint8_t *cart, uint32_t len)
{
	// Harmony/Melody images start with the ARM driver, whose first word is the reset
	// vector: an unconditional ARM instruction loading PC ("LDR PC, [PC, #n]" or
	// "MOV PC, #n")
	return len >= 4 && (cart[3] >> 4) == 0x0e && (cart[1] >> 4) == 0x0f && ((cart[3] == 0xe5 && cart[2] == 0x9f) || (cart[3] == 0xe3 && cart[2] == 0xa0));
}

int vcs_cart_slot_device::count_abs_accesses(const uint8_t *cart, uint32_t len, uint16_t first, uint16_t last)
{
	return count_abs_accesses_masked(cart, len, 0x1fff, first, last);
}

// the address in the instruction is ANDed with mask (without folding mirrors first)
int vcs_cart_slot_device::count_abs_accesses_masked(const uint8_t *cart, uint32_t len, uint16_t mask, uint16_t first, uint16_t last)
{
	// opcodes of instructions with an absolute (or absolute indexed) address that are used to
	// access bankswitching hotspots: LDA, LDX, LDY, STA, STX, STY, BIT, CMP, CPX, CPY, NOP
	static const uint8_t opcodes[] = { 0xad, 0xbd, 0xb9, 0xae, 0xbe, 0xac, 0xbc, 0x8d, 0x9d, 0x99, 0x8e, 0x8c, 0x2c, 0xcd, 0xdd, 0xd9, 0xec, 0xcc, 0x0c };
	int found = 0;

	for (uint32_t i = 0; i + 2 < len; i++)
	{
		if (std::find(std::begin(opcodes), std::end(opcodes), cart[i]) != std::end(opcodes))
		{
			uint16_t const address = (cart[i + 1] | (cart[i + 2] << 8)) & mask;
			if (address >= first && address <= last)
				found++;
		}
	}
	return found;
}

bool vcs_cart_slot_device::detect_32in1(const uint8_t *cart, uint32_t len)
{
	// 32 2K games: (nearly) every 2K block has a reset vector pointing to its own startup code
	static const uint8_t first_opcodes[] = { 0x78, 0xd8, 0xa2, 0xa0, 0xa9, 0x4c, 0x20, 0x85, 0xa5 };
	if (len != 0x10000)
		return 0;

	int games = 0;
	for (uint32_t block = 0; block < len; block += 0x800)
	{
		uint16_t const reset = (cart[block + 0x7fd] << 8) | cart[block + 0x7fc];
		uint8_t const opcode = cart[block + (reset & 0x7ff)];
		if ((reset & 0x1000) && std::find(std::begin(first_opcodes), std::end(first_opcodes), opcode) != std::end(first_opcodes))
			games++;
	}
	return games >= 30;
}

bool vcs_cart_slot_device::detect_8in1(const uint8_t *cart, uint32_t len)
{
	// 8 8K games with F8 bankswitching: every 8K accesses the F8 hotspots
	if (len != 0x10000 || count_abs_accesses(cart, len, 0x1fe0, 0x1fef) > count_abs_accesses(cart, len, 0x1ff8, 0x1ff9))
		return 0;

	for (uint32_t game = 0; game < len; game += 0x2000)
		if (!count_abs_accesses(&cart[game], 0x2000, 0x1ff8, 0x1ff9))
			return 0;
	return 1;
}

bool vcs_cart_slot_device::detect_modeSB(const uint8_t *cart, uint32_t len)
{
	// 128K or 256K, banks selected by accessing 0800-083f: compare with the DF/BF hotspots
	if (len != 0x20000 && len != 0x40000)
		return 0;

	int const banks = len / 0x1000;
	int const sb = count_abs_accesses_masked(cart, len, 0xffff, 0x0800, 0x0800 + banks - 1);
	int const other = (len == 0x20000) ? count_abs_accesses(cart, len, 0x1fc0, 0x1fdf) : count_abs_accesses(cart, len, 0x1f80, 0x1fbf);
	return sb >= 3 && sb > other;
}

bool vcs_cart_slot_device::detect_mode0840(const uint8_t *cart, uint32_t len)
{
	// both 0800 and 0840 are accessed (often one of them shows up in other 8K games)
	return len == 0x2000 && count_abs_accesses(cart, len, 0x0800, 0x0800) && count_abs_accesses(cart, len, 0x0840, 0x0840);
}

bool vcs_cart_slot_device::detect_modeX07(const uint8_t *cart, uint32_t len)
{
	// banks are selected by accessing 0 1xxx nnnn 1101: look for 080d, 081d, ... 08fd in
	// the code (graphics data and other bankswitching code rarely have more than a few)
	return len == 0x10000 && count_abs_accesses_masked(cart, len, 0xff0f, 0x080d, 0x080d) >= 6;
}

bool vcs_cart_slot_device::detect_modeFE(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signatures[][5] =  {
									{ 0x20, 0x00, 0xd0, 0xc6, 0xc5 },
									{ 0x20, 0xc3, 0xf8, 0xa5, 0x82 },
									{ 0xd0, 0xfb, 0x20, 0x73, 0xfe },
									{ 0x20, 0x00, 0xf0, 0x84, 0xd6 }
	};

	if (len == 0x2000)
	{
		for (int i = 0; i < len - sizeof signatures[0]; i++)
		{
			for (int j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j], sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeE0(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signatures[][3] =  {
									{ 0x8d, 0xe0, 0x1f },
									{ 0x8d, 0xe0, 0x5f },
									{ 0x8d, 0xe9, 0xff },
									{ 0xad, 0xe9, 0xff },
									{ 0xad, 0xed, 0xff },
									{ 0xad, 0xf3, 0xbf }
	};

	if (len == 0x2000)
	{
		for (int i = 0; i < len - sizeof signatures[0]; i++)
		{
			for (int j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j], sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeCV(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0x9d, 0xff, 0xf3 },
									{ 0x99, 0x00, 0xf4 }
	};

	if (len == 0x0800 || len == 0x1000)
	{
		for (int i = 0; i < len - sizeof signatures[0]; i++)
		{
			for (int j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j], sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeFV(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signatures[][3] = { { 0x2c, 0xd0, 0xff } };

	if (len == 0x2000)
	{
		for (int i = 0; i < len - sizeof signatures[0]; i++)
		{
			for (int j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j], sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeJVP(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signatures[][4] = {
									{ 0x2c, 0xc0, 0xef, 0x60 },
									{ 0x8d, 0xa0, 0x0f, 0xf0 }
	};

	if (len == 0x4000 || len == 0x2000)
	{
		for (int i = 0; i < len - sizeof signatures[0]; i++)
		{
			for (int j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j], sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeE7(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0xad, 0xe5, 0xff },
									{ 0x8d, 0xe7, 0xff }
	};

	if (len == 0x2000 || len == 0x3000 || len == 0x4000)
	{
		for (int i = 0; i < len - sizeof signatures[0]; i++)
		{
			for (int j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j], sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_modeUA(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signature[3] = { 0x8d, 0x40, 0x02 };

	if (len == 0x2000)
	{
		for (int i = 0; i < len - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature, sizeof signature))
			{
				numfound = 1;
			}
		}

		// or any accesses to both hotspots
		if (count_abs_accesses(cart, len, 0x0220, 0x0220) && count_abs_accesses(cart, len, 0x0240, 0x0240))
			numfound = 1;
	}
	if (numfound)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_8K_mode3F(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signature1[4] = { 0xa9, 0x01, 0x85, 0x3f };
	static const unsigned char signature2[4] = { 0xa9, 0x02, 0x85, 0x3f };
	// have to look for two signatures because 'not boulderdash' gives false positive otherwise

	if (len == 0x2000)
	{
		for (int i = 0; i < len - sizeof signature1; i++)
		{
			if (!memcmp(&cart[i], signature1, sizeof signature1))
			{
				numfound |= 0x01;
			}
			if (!memcmp(&cart[i], signature2, sizeof signature2))
			{
				numfound |= 0x02;
			}
		}
	}
	if (numfound == 0x03)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_32K_mode3F(const uint8_t *cart, uint32_t len)
{
	int numfound = 0;
	static const unsigned char signature[4] = { 0xa9, 0x0e, 0x85, 0x3f };

	if (len >= 0x8000)
	{
		for (int i = 0; i < len - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature, sizeof signature))
			{
				numfound++;
			}
		}
	}
	if (numfound > 1)
		return 1;
	return 0;
}

bool vcs_cart_slot_device::detect_super_chip(const uint8_t *cart, uint32_t len)
{
	static const unsigned char signatures[][5] = {
									{ 0xa2, 0x7f, 0x9d, 0x00, 0xf0 }, // dig dug
									{ 0xae, 0xf6, 0xff, 0x4c, 0x00 } // off the wall
	};

	if (len == 0x4000)
	{
		for (int i = 0; i < len - sizeof signatures[0]; i++)
		{
			for (auto & signature : signatures)
			{
				if (!memcmp(&cart[i], &signature, sizeof signatures[0]))
				{
					return 1;
				}
			}
		}
	}
	for (int i = 0x1000; i < len; i += 0x1000)
	{
		if (memcmp(cart, cart + i, 0x100))
		{
			return 0;
		}
	}
	/* Check the reset vector does not point into the super chip RAM area */
	if ((((cart[0x0ffd] << 8) | cart[0x0ffc]) & 0x0fff) < 0x0100)
	{
		return 0;
	}
	return 1;
}


// ARM based Harmony/Melody formats: the driver at the start of the image names the format
bool vcs_cart_slot_device::detect_harmony(const uint8_t *cart, uint32_t len)
{
	auto const contains = [cart, len] (uint32_t end, std::string_view signature)
	{
		end = std::min(end, len);
		auto const same = [] (uint8_t a, char b) { return a == uint8_t(b); };
		return std::search(cart, cart + end, signature.begin(), signature.end(), same) != cart + end;
	};

	// DPC+: 3K driver, 24K of banks, 4K of display data and 1K of frequencies
	if (len == 0x8000 && contains(0xc00, "DPC+"))
		return true;

	// CDFJ+: up to 512K
	if (len >= 0x8000 && contains(0x800, "PLUSCDFJ"))
		return true;

	// CDF and CDFJ: "CDF" and the version, three times, in the 2K driver
	if (len == 0x8000)
	{
		for (uint32_t i = 0; i + 12 <= 0x800; i++)
			if (!memcmp(&cart[i], "CDF", 3) && !memcmp(&cart[i], &cart[i + 4], 4) && !memcmp(&cart[i], &cart[i + 8], 4))
				return true;
	}

	return false;
}


/*-------------------------------------------------
 identify_cart_type - code to detect cart type from
 fullpath
 -------------------------------------------------*/

// 4in1 is not currently detected from fullpath...
int vcs_cart_slot_device::identify_cart_type(const uint8_t *ROM, uint32_t len)
{
	int type = 0xff;

	// auto-detect bank mode
	if (detect_ss_tape(ROM, len))
		type = A26_SS;
	else if (detect_harmony(ROM, len))
		type = A26_HARMONY;
	else if (detect_modeDC(ROM, len))
		type = A26_DC;
	else if (detect_modeX07(ROM, len))
		type = A26_X07;
	else if (detect_modeSB(ROM, len))
		type = A26_SB;
	else if (detect_32in1(ROM, len))
		type = A26_32IN1;
	else if (detect_8in1(ROM, len))
		type = A26_8IN1;
	else if (detect_mode3E(ROM, len))
		type = A26_3E;
	else if (detect_modeFE(ROM, len))
		type = A26_FE;
	else if (detect_modeSS(ROM, len))
		type = A26_SS;
	else if (detect_modeE0(ROM, len))
		type = A26_E0;
	else if (detect_modeCV(ROM, len))
		type = A26_CV;
	else if (detect_modeFV(ROM, len))
		type = A26_FV;
	else if (detect_modeJVP(ROM, len))
		type = A26_JVP;
	else if (detect_modeUA(ROM, len))
		type = A26_UA;
	else if (detect_mode0840(ROM, len))
		type = A26_0840;
	else if (detect_mode3EP(ROM, len))
		type = A26_3EP;
	else if (detect_any_3E(ROM, len))
		type = A26_3E;
	else if (detect_8K_mode3F(ROM, len))
		type = A26_3F;
	else if (detect_32K_mode3F(ROM, len))
		type = A26_3F;
	else if (detect_any_3F(ROM, len))
		type = A26_3F;
	else if (detect_modeE7(ROM, len))
		type = A26_E7;
	else if (detect_modeFA2(ROM, len))
		type = A26_FA2;
	else if (detect_snowhite(ROM, len))
		type = A26_F8SW;
	else if (detect_f8_bank1_start(ROM, len))
		type = A26_F8SW;

	// otherwise, choose based on size (-1 if the size is not a known one)
	if (type == 0xff)
		type = default_type_for_size(len);

	return type;
}

/*-------------------------------------------------
 default_type_for_size - the usual board for a
 cartridge of a given size, when nothing better is
 known (-1 for sizes no board uses)
 -------------------------------------------------*/

int vcs_cart_slot_device::default_type_for_size(uint32_t len)
{
	switch (len)
	{
		case 0x800:
		case 0x1000:
			return A26_2K_4K;
		case 0x2000:
			return A26_F8;
		case 0x28ff:
		case 0x2900:
			return A26_DPC;
		case 0x3000:
			return A26_FA;
		case 0x4000:
			return A26_F6;
		case 0x6000:
		case 0x7000:
		case 0x7400:
			return A26_FA2;
		case 0x8000:
			return A26_F4;
		case 0x10000:
			return A26_EF;
		case 0x20000:
			return A26_DF;
		case 0x40000:
			return A26_BF;
		case 0x80000:
			return A26_3F;
		default:
			return -1;
	}
}

/*-------------------------------------------------
 get default card software
 -------------------------------------------------*/

std::string vcs_cart_slot_device::get_default_card_software(get_default_card_software_hook &hook) const
{
	if (hook.image_file())
	{
		uint64_t len;
		hook.image_file()->length(len); // FIXME: check error return, guard against excessively large files
		std::vector<uint8_t> rom(len);

		read(*hook.image_file(), &rom[0], len); // FIXME: check error return or read returning short

		int const type = identify_cart_type(&rom[0], len);
		if (type < 0)
			logerror("Unrecognized cartridge (%u bytes), assuming 2K/4K\n", uint32_t(len));
		char const *const slot_string = vcs_get_slot(type);

		return std::string(slot_string);
	}
	else
		return software_get_default_slot("a26_2k_4k");
}
