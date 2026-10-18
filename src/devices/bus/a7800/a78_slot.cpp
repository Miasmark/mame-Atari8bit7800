// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/***********************************************************************************************************


    Atari 7800 cart emulation
    (through slot devices)

    Emulation of the cartslot for Atari 7800

    Quoting "ATARI 7800 BANKSWITCHING GUIDE" (by Eckhard Stolberg):
    7800 games can use the memory from $0400 to $047f, from $0500
    to $17ff and from $2800 to $ffff, but only the High-Score cart
    uses anything below $4000. It has 4KB of ROM at $3000-$3fff
    and 2KB of battery-backed RAM at $1000-$17ff.

    Accordingly, we use the following handlers:
    - read_04xx/write_04xx for accesses in the $0400 to $047f range
    - read_10xx/write_10xx for accesses in the $1000 to $17ff range
    - read_30xx/write_30xx for accesses in the $3000 to $3fff range
    - read_40xx/write_40xx for accesses in the $4000 to $ffff range
    even if not all carts use all of them (in particular no cart type
    seems to use access to the ranges $0500 to $0fff and $2800 to $2fff)


 ***********************************************************************************************************/

#include "emu.h"
#include "a78_slot.h"

#include "emuopts.h"
#include "romload.h"
#include "softlist_dev.h"

#include "hashing.h"
#include "multibyte.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(A78_CART_SLOT, a78_cart_slot_device, "a78_cart_slot", "Atari 7800 Cartridge Slot")


//-------------------------------------------------
//  device_vcs_cart_interface - constructor
//-------------------------------------------------

device_a78_cart_interface::device_a78_cart_interface (const machine_config &mconfig, device_t &device)
	: device_interface(device, "a78cart")
	, m_rom(nullptr)
	, m_rom_size(0)
	, m_base_rom(0x8000)
	, m_bank_mask(0)
{
}


//-------------------------------------------------
//  ~device_a78_cart_interface  - destructor
//-------------------------------------------------

device_a78_cart_interface::~device_a78_cart_interface ()
{
}

//-------------------------------------------------
//  rom_alloc - alloc the space for the cart
//-------------------------------------------------

void device_a78_cart_interface::rom_alloc(uint32_t size)
{
	if (m_rom == nullptr)
	{
		m_rom = device().machine().memory().region_alloc(device().subtag("^cart:rom"), size, 1, ENDIANNESS_LITTLE)->base();
		m_rom_size = size;

		// setup other helpers
		if ((size / 0x4000) & 1) // compensate for SuperGame carts with 9 x 16K banks (to my knowledge no other cart has m_bank_mask != power of 2)
			m_bank_mask = (size / 0x4000) - 2;
		else
			m_bank_mask = (size / 0x4000) - 1;

		// the rom is mapped to the top of the memory area
		// so we store the starting point of data to simplify
		// the access handling
		m_base_rom = 0x10000 - size;
	}
}

//-------------------------------------------------
//  ram_alloc - alloc the space for the on-cart RAM
//-------------------------------------------------

void device_a78_cart_interface::ram_alloc(uint32_t size)
{
	m_ram.resize(size);
	device().save_item(NAME(m_ram));
}


//-------------------------------------------------
//  ram_alloc - alloc the space for the on-cart RAM
//-------------------------------------------------

void device_a78_cart_interface::nvram_alloc(uint32_t size)
{
	m_nvram.resize(size);
	device().save_item(NAME(m_nvram));
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  a78_cart_slot_device - constructor
//-------------------------------------------------
a78_cart_slot_device::a78_cart_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, A78_CART_SLOT, tag, owner, clock)
	, device_cartrom_image_interface(mconfig, *this)
	, device_slot_interface(mconfig, *this)
	, m_cart(nullptr)
	, m_type(0)
	, m_vcs_image(nullptr)
	, m_savekey(false)
	, m_hsc_save(false)
	, m_expansion(false)
	, m_origin(nullptr)
{
}


//-------------------------------------------------
//  a78_cart_slot_device - destructor
//-------------------------------------------------

a78_cart_slot_device::~a78_cart_slot_device()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a78_cart_slot_device::device_start()
{
	m_cart = dynamic_cast<device_a78_cart_interface  *>(get_card_device());
}



/*-------------------------------------------------
 call load
 -------------------------------------------------*/

int a78_cart_slot_device::validate_header(int head, bool log) const
{
	switch (head & 0x3d)
	{
		case 0x05:
			if (log)
			{
				osd_printf_info("POKEY + RAM at $4000 (Header 0x05)\n");
				osd_printf_info("Disabling POKEY\n");
			}
			head &= ~0x01;
			break;
		case 0x09:
			if (log)
			{
				osd_printf_info("POKEY + Bank 0 of 144K ROM  at $4000 (Header 0x09)\n");
				osd_printf_info("Disabling POKEY\n");
			}
			head &= ~0x01;
			break;
		case 0x11:
			if (log)
			{
				osd_printf_info("POKEY + Bank 6 ROM at $4000 (Header 0x11)\n");
				osd_printf_info("Disabling POKEY\n");
			}
			head &= ~0x01;
			break;
		case 0x21:
			if (log)
			{
				osd_printf_info("POKEY + banked RAM at $4000 (Header 0x21)\n");
				osd_printf_info("Disabling POKEY\n");
			}
			head &= ~0x01;
			break;
		case 0x0c:
			if (log)
			{
				osd_printf_info("RAM + Bank 0 of 144K ROM at $4000 (Header 0x0c)\n");
				osd_printf_info("Disabling RAM\n");
			}
			head &= ~0x04;
			break;
		case 0x14:
			if (log)
			{
				osd_printf_info("RAM + Bank 6 ROM at $4000 (Header 0x14)\n");
				osd_printf_info("Disabling RAM\n");
			}
			head &= ~0x04;
			break;
		case 0x24:
			if (log)
			{
				osd_printf_info("RAM + Banked RAM at $4000 (Header 0x24)\n");
				osd_printf_info("Disabling RAM\n");
			}
			head &= ~0x04;
			break;
		case 0x18:
			if (log)
			{
				osd_printf_info("Bank 0 of 144K ROM + Bank 6 ROM at $4000 (Header 0x18)\n");
				osd_printf_info("Disabling Bank 0 ROM\n");
			}
			head &= ~0x08;
			break;
		case 0x28:
			if (log)
			{
				osd_printf_info("Bank 0 of 144K ROM + Banked RAM at $4000 (Header 0x28)\n");
				osd_printf_info("Disabling Bank 0 ROM\n");
			}
			head &= ~0x08;
			break;
		case 0x30:
			if (log)
			{
				osd_printf_info("Bank 6 ROM + banked RAM at $4000 (Header 0x30)\n");
				osd_printf_info("Disabling Bank 6 ROM\n");
			}
			head &= ~0x10;
			break;
	}

	if ((head & 0x3c) && !(head & 0x02))
	{
		if (log)
		{
			osd_printf_info("SuperCart bankswitch detected at $4000, with no SuperCart bit (Header 0x%X)\n", head);
			osd_printf_info("Enablig SuperCart bankswitch\n");
		}
		head |= 0x02;
	}

	if ((head & 0xff00) == 0x100 && (head & 0xff))
	{
		if (log)
		{
			osd_printf_info("Bankswitch detected for an Activision cart (Header 0x%X)\n", head);
			osd_printf_info("Disabling bankswitch\n");
		}
		head &= 0xff00;
	}

	if ((head & 0xff00) == 0x200 && (head & 0xff))
	{
		if (log)
		{
			osd_printf_info("Bankswitch detected for an Absolute cart (Header 0x%X)\n", head);
			osd_printf_info("Disabling bankswitch\n");
		}
		head &= 0xff00;
	}

	// bankset ($2000), bank RAM ($4000) and POKEY at $0800 ($8000) are handled by a78_special_type,
	// and a second POKEY at $0440 ($0400, beside $0450's $0040) by a78_dual_pokey; anything else
	// above the Activision/Absolute bits is still unsupported
	int const known = ((head & 0x0440) == 0x0440) ? (head & ~0x0400) : head;
	if ((known & 0xff00) > 0x300 && (known & 0xff00 & ~0xe000) != 0 && !(known & 0xe000))
	{
		if (log)
		{
			osd_printf_info("Unsupported mapper, please contact MAMEdevs (Header 0x%X)\n", head);
			osd_printf_info("Disabling special bits\n");
		}
		head &= 0x00ff;
	}

	return head;
}


//-------------------------------------------------
//  A78 PCBs
//-------------------------------------------------

struct a78_slot
{
	int                     pcb_id;
	const char              *slot_option;
};

#define A78_POKEY0450 0x20
#define A78_POKEY0440 0x40  // added to a POKEY0450 type: a second POKEY at $0440

// a second POKEY at $0440 beside the one at $0450: header bit $0400, with $0040
static bool a78_dual_pokey(int mapper, int type)
{
	return (mapper & 0x0440) == 0x0440
			&& (type == A78_TYPE0_POK450 || type == A78_TYPE6_POK450 || type == A78_TYPEA_POK450 || type == A78_VERSA_POK450);
}

// Here, we take the feature attribute from .xml (i.e. the PCB name) and we assign a unique ID to it
static const a78_slot slot_list[] =
{
	{ A78_TYPE0,      "a78_rom" },
	{ A78_TYPE1,      "a78_pokey" },
	{ A78_TYPE2,      "a78_sg" },
	{ A78_TYPE3,      "a78_sg_pokey" },
	{ A78_TYPE6,      "a78_sg_ram" },
	{ A78_TYPEA,      "a78_sg9" },
	{ A78_TYPE8,      "a78_mram" },
	{ A78_ABSOLUTE,   "a78_abs" },
	{ A78_ACTIVISION, "a78_act" },
	{ A78_HSC,        "a78_hsc" },
	{ A78_XB_BOARD,   "a78_xboard" },
	{ A78_XM_BOARD,   "a78_xm" },
	{ A78_MEGACART,   "a78_megacart" },
	{ A78_VERSABOARD, "a78_versa" },
	{ A78_TYPE0_POK450, "a78_p450_t0" },
	{ A78_TYPE1_POK450, "a78_p450_t1" },
	{ A78_TYPE6_POK450, "a78_p450_t6" },
	{ A78_TYPEA_POK450, "a78_p450_ta" },
	{ A78_VERSA_POK450, "a78_p450_vb" },
	{ A78_TYPE0_POK450_440, "a78_p440_t0" },
	{ A78_TYPE6_POK450_440, "a78_p440_t6" },
	{ A78_TYPEA_POK450_440, "a78_p440_ta" },
	{ A78_VERSA_POK450_440, "a78_p440_vb" },
	{ A78_TYPE0_POK800, "a78_p800_t0" },
	{ A78_TYPE1_POK800, "a78_p800_t1" },
	{ A78_TYPE2_POK800, "a78_p800_t2" },
	{ A78_TYPE6_POK800, "a78_p800_t6" },
	{ A78_TYPEA_POK800, "a78_p800_ta" },
	{ A78_BANKSET,                   "a78_bankset" },
	{ A78_BANKSET_POK800,            "a78_bankset_p800" },
	{ A78_BANKSET_POK4000,           "a78_bankset_p4000" },
	{ A78_BANKSET_SG,                "a78_bankset_sg" },
	{ A78_BANKSET_SG_POK800,         "a78_bankset_sg_p800" },
	{ A78_BANKSET_SG_BANKRAM,        "a78_bankset_sg_bankram" },
	{ A78_BANKSET_SG_BANKRAM_POK800, "a78_bankset_sg_bankram_p800" },
	{ A78_BANKSET_BANKRAM,           "a78_bankset_bankram" },
	{ A78_BANKSET_BANKRAM_POK800,    "a78_bankset_bankram_p800" },
	{ A78_BANKSET_52K,               "a78_bankset_52k" },
	{ A78_BANKSET_52K_POK4000,       "a78_bankset_52k_p4000" }
};

// Bankset carts (header bit $2000) and carts with a POKEY at $0800 (header bit $8000), which
// the plain header switch does not know. -1 if this header is neither.
static int a78_special_type(int mapper, uint64_t len)
{
	switch (mapper & 0xe02e)
	{
	case 0x2000:    // flat bankset, with or without POKEY at $4000; 52K halves reach down to $3000
		if (BIT(mapper, 0))
			return (len >= 0x1a000) ? A78_BANKSET_52K_POK4000 : A78_BANKSET_POK4000;
		return (len >= 0x1a000) ? A78_BANKSET_52K : A78_BANKSET;
	case 0xa000: return A78_BANKSET_POK800;
	case 0x6000: return A78_BANKSET_BANKRAM;
	case 0xe000: return A78_BANKSET_BANKRAM_POK800;
	case 0x2002: return A78_BANKSET_SG;
	case 0x6002: return A78_BANKSET_SG_BANKRAM;
	case 0xa002: return A78_BANKSET_SG_POK800;
	case 0xe002: return A78_BANKSET_SG_BANKRAM_POK800;
	case 0x8000: return BIT(mapper, 0) ? A78_TYPE1_POK800 : A78_TYPE0_POK800;
	case 0x8002: return A78_TYPE2_POK800;
	case 0x8006: return A78_TYPE6_POK800;
	case 0x800a: return A78_TYPEA_POK800;
	}
	return -1;
}

static int a78_get_pcb_id(const char *slot)
{
	for (auto & elem : slot_list)
	{
		if (!strcmp(elem.slot_option, slot))
			return elem.pcb_id;
	}

	return 0;
}

// Activision images come in two orders. The software list's (and the "(OM)" .a78 files') are the
// EPROM's, which a78_rom_act_device reads; the usual .a78 order ("(AM)", the a7800 fork's) has each
// pair of 8K blocks exchanged, so read MAME's way its vectors land in a block of $00 or $FF and the
// BIOS never hands over. Put such an image back in EPROM order, but only when its reset vector says
// so: one that already boots is left alone.
static void activision_eprom_order(uint8_t *rom, uint32_t len)
{
	if (len != 0x20000)
		return;
	auto usable = [] (uint16_t v) { return v >= 0x4000 && v != 0xffff; };
	uint16_t const eprom = get_u16le(&rom[0x1fffc]);
	uint16_t const usual = get_u16le(&rom[0x1dffc]);  // $FFFC is in block 14 in the usual order
	if (!usable(eprom) && usable(usual))
	{
		for (uint32_t b = 0; b < len; b += 0x4000)
			std::swap_ranges(&rom[b], &rom[b + 0x2000], &rom[b + 0x2000]);
		osd_printf_info("Activision image in the usual .a78 block order (reset $%04X): read in EPROM order\n", usual);
	}
}

// the test the 7800 BIOS makes before it runs a cartridge as a 7800 one: $FFF9 AND $0B is 3, and the
// top nibble is where the signed area starts, which has to be in the cartridge ($FFF9 is at check)
static bool a78_bios_check(uint8_t check, uint32_t len)
{
	uint32_t const signed_start = (check & 0xf0) << 8;
	return (check & 0x0b) == 0x03 && signed_start >= std::max<uint32_t>(0x10000 - std::min<uint32_t>(len, 0xc000), 0x4000);
}

// the High Score Cartridge's ROM (4K), known without the software list too
bool a78_cart_slot_device::is_hsc_image(const uint8_t *data, uint32_t len) const
{
	if (len != 0x1000)
		return false;
	uint32_t const crc = util::crc32_creator::simple(data, len);
	return crc == 0x9be408d3 || softlist_type(data, len) == A78_HSC;
}

// the image this slot last passed on to the 2600 cartridge slot
std::string a78_cart_slot_device::s_vcs_forwarded;

// what headerless_type() returns for an image that is not a 7800 one, or a 7800 one too
// big to say how its board works without a header
enum
{
	HEADERLESS_2600 = -1,
	HEADERLESS_UNKNOWN = -2
};

// The board of a known cartridge dump, from the software lists of this slot's interface (which
// covers images without a header, and the High Score Cartridge's own image); -1 if none knows it.
int a78_cart_slot_device::softlist_type(const uint8_t *data, uint32_t len) const
{
	uint32_t const crc = util::crc32_creator::simple(data, len);
	for (software_list_device &swlist : software_list_device_enumerator(mconfig().root_device()))
	{
		for (const software_info &sw : swlist.get_info())
		{
			for (const software_part &part : sw.parts())
			{
				if (!part.matches_interface(image_interface()))
					continue;
				for (const rom_entry &entry : part.romdata())
				{
					uint32_t entry_crc;
					if (ROMENTRY_ISFILE(entry) && ROM_GETLENGTH(entry) == len
							&& util::hash_collection(entry.hashdata()).crc(entry_crc) && entry_crc == crc)
					{
						char const *const slot = part.feature("slot");
						return slot ? a78_get_pcb_id(slot) : A78_TYPE0;
					}
				}
			}
		}
	}
	return -1;
}

// An image without a header: a known 7800 dump, a 7800 image the BIOS would run, or a 2600 image.
int a78_cart_slot_device::headerless_type(const uint8_t *data, uint32_t len) const
{
	if (is_hsc_image(data, len))
		return A78_HSC;
	if (int const type = softlist_type(data, len); type >= 0)
		return type;

	// A 7800 image passes the BIOS's own test: of 1309 7800 images (header removed) this takes 1271
	// (not tiny demos, BankSet tests or Activision's .a78 block order, looked for below), and none
	// of 279 2600 ones.
	if (len < 0x4000 || (len % 0x4000))
		return HEADERLESS_2600;
	if (!a78_bios_check(data[len - 7], len))
	{
		// Activision's 128K in the usual .a78 order: $FFF9 is in block 14
		if (len == 0x20000 && a78_bios_check(data[0x1dff9], len))
			return A78_ACTIVISION;
		return HEADERLESS_2600;
	}

	// a flat image fills $4000-$FFFF from the top; SuperGame boards come as 128K, or 144K with
	// the extra bank at $4000
	if (len <= 0xc000)
		return A78_TYPE0;
	if (len == 0x20000)
		return A78_TYPE2;
	if (len == 0x24000)
		return A78_TYPEA;
	return HEADERLESS_UNKNOWN;
}

static const char *a78_get_slot(int type)
{
	for (auto & elem : slot_list)
	{
		if (elem.pcb_id == type)
			return elem.slot_option;
	}

	return "a78_rom";
}

std::pair<std::error_condition, std::string> a78_cart_slot_device::call_load()
{
	m_savekey = m_hsc_save = m_expansion = false;
	m_origin = nullptr;

	// without a board, the image is a 2600 one that the 2600 cartridge slot loads
	if (m_cart)
	{
		uint32_t len;

		if (loaded_through_softlist())
		{
			const char *pcb_name;
			bool has_ram = get_software_region("ram") ? true : false;
			bool has_nvram = get_software_region("nvram") ? true : false;
			len = get_software_region_length("rom");

			m_cart->rom_alloc(len);
			memcpy(m_cart->get_rom_base(), get_software_region("rom"), len);

			if ((pcb_name = get_feature("slot")) != nullptr)
				m_type = a78_get_pcb_id(pcb_name);
			else
				m_type = A78_TYPE0;
			m_origin = "board from the software list";

			if (has_ram)
				m_cart->ram_alloc(get_software_region_length("ram"));
			if (has_nvram)
			{
				m_cart->nvram_alloc(get_software_region_length("nvram"));
				battery_load(m_cart->get_nvram_base(), get_software_region_length("nvram"), 0xff);
			}
		}
		else
		{
			// Load and check the header
			int mapper;
			uint8_t head[128] = { };
			fread(head, 128);

			auto err = verify_header(head);
			if (err.first)
			{
				// no header: a dump of the cartridge's ROM alone
				len = length();
				std::vector<uint8_t> data(len);
				fseek(0, SEEK_SET);
				fread(&data[0], len);
				int const type = headerless_type(&data[0], len);
				if (type == HEADERLESS_2600)
					return std::make_pair(image_error::INVALIDIMAGE, "This is a 2600 cartridge: it goes in the 2600 cartridge slot (-cart2600)");
				if (type == HEADERLESS_UNKNOWN)
					return std::make_pair(image_error::INVALIDIMAGE, "A 7800 image of this size needs its .a78 header to say what board it is on");

				m_type = type;
				if (is_hsc_image(&data[0], len))
					m_origin = "the High Score Cart's own ROM";
				else if (softlist_type(&data[0], len) >= 0)
					m_origin = "no header, board from the software list";
				else
					m_origin = "no header, board detected from the image";
				logerror("Image without a header: cart type 0x%x\n", m_type);
				m_cart->rom_alloc(len);
				memcpy(m_cart->get_rom_base(), &data[0], len);
				if (m_type == A78_ACTIVISION)
					activision_eprom_order(m_cart->get_rom_base(), len);
				alloc_board_memory();
				return std::make_pair(std::error_condition(), std::string());
			}

			len = get_u32be(&head[49]);
			if (len + 128 > length())
			{
				logerror("Invalid length in the header. The game might be corrupted.\n");
				len = length() - 128;
			}

			// let's try to auto-fix some common errors in the header
			mapper = validate_header(get_u16be(&head[53]), true);

			switch (mapper & 0x2e)
			{
				case 0x0000:
					m_type = BIT(mapper, 0) ? A78_TYPE1 : A78_TYPE0;
					break;
				case 0x0002:
					m_type = BIT(mapper, 0) ? A78_TYPE3 : A78_TYPE2;
					break;
				case 0x0006:
					m_type = A78_TYPE6;
					break;
				case 0x000a:
					m_type = A78_TYPEA;
					break;
				case 0x0022:
				case 0x0026:
					if (len > 0x40000)
						m_type = A78_MEGACART;
					else
						m_type = A78_VERSABOARD;
					break;
			}

			// check if cart has a POKEY at $0450 (typically a VersaBoard variant)
			if (mapper & 0x40)
			{
				if (m_type != A78_TYPE2)
				{
					m_type &= ~0x02;
					m_type += A78_POKEY0450;
				}
			}
			if (a78_dual_pokey(mapper, m_type))
				m_type += A78_POKEY0440;

			if (int const special = a78_special_type(mapper, len); special >= 0)
				m_type = special;

			// check special bits, which override the previous
			if ((mapper & 0xff00) == 0x0100)
				m_type = A78_ACTIVISION;
			else if ((mapper & 0xff00) == 0x0200)
				m_type = A78_ABSOLUTE;
			// (for now) mirror ram implies no bankswitch format is used
			else if ((mapper & 0x0080) == 0x0080)
				m_type = A78_TYPE8;


			logerror("Cart type: 0x%x\n", m_type);
			m_origin = "board from the .a78 header";

			m_hsc_save = BIT(head[58], 0);
			if (head[58] == 1)
			{
				osd_printf_info("This cart supports external NVRAM using HSC.\n");
				osd_printf_info("Run it with the High Score Cart mounted to exploit this feature.\n");
			}
			// a SaveKey or AtariVox in port 2: header byte 58 bit 1, or the controller type of port 2
			m_savekey = BIT(head[58], 1) || head[56] == 10;
			if (m_savekey)
			{
				osd_printf_info("This cart saves to a SaveKey (or AtariVox) in controller port 2.\n");
				osd_printf_info("It is plugged in there unless Machine Configuration says otherwise.\n");
			}

			m_expansion = head[63] != 0;
			if (head[63])
			{
				osd_printf_info("This cart requires XBoarD / XM expansion\n");
				osd_printf_info("Run it through the expansion to exploit this feature.\n");
			}

			internal_header_logging(head, length());

			m_cart->rom_alloc(len);
			fread(m_cart->get_rom_base(), len);

			// the High Score Cartridge's own image: its header says nothing of it
			if (is_hsc_image(m_cart->get_rom_base(), len))
			{
				m_type = A78_HSC;
				m_origin = "the High Score Cart's own ROM";
			}

			if (m_type == A78_ACTIVISION)
				activision_eprom_order(m_cart->get_rom_base(), len);

			alloc_board_memory();
		}

		//printf("Type: %s\n", a78_get_slot(m_type));
	}
	return std::make_pair(std::error_condition(), std::string());
}

// RAM and battery-backed RAM on the board of an image loaded from a file
void a78_cart_slot_device::alloc_board_memory()
{
	if (m_type == A78_TYPE6 || m_type == A78_TYPE8)
		m_cart->ram_alloc(0x4000);
	if (m_type == A78_MEGACART || (m_type >= A78_VERSABOARD && m_type <= A78_VERSA_POK450)
			|| (m_type >= A78_TYPE0_POK450_440 && m_type <= A78_VERSA_POK450_440))
		m_cart->ram_alloc(0x8000);
	// bankset bank RAM: 16K for the CPU at $4000 and 16K for MARIA
	if (m_type == A78_BANKSET_SG_BANKRAM || m_type == A78_BANKSET_SG_BANKRAM_POK800 ||
			m_type == A78_BANKSET_BANKRAM || m_type == A78_BANKSET_BANKRAM_POK800)
		m_cart->ram_alloc(0x8000);
	if (m_type == A78_XB_BOARD || m_type == A78_XM_BOARD)
		m_cart->ram_alloc(0x20000);
	if (m_type == A78_HSC || m_type == A78_XM_BOARD)
	{
		m_cart->nvram_alloc(0x800);
		battery_load(m_cart->get_nvram_base(), 0x800, 0xff);
	}
}


/*-------------------------------------------------
 media_details - the board, and how it was told
 -------------------------------------------------*/

std::string a78_cart_slot_device::media_details() const
{
	if (!m_cart)
		return "2600 cartridge: it runs in the 2600 cartridge slot (cart2600)";

	std::string details(device_cartrom_image_interface::media_details());
	std::string notes(m_origin ? m_origin : "");
	auto const note = [&notes] (const char *text) { if (!notes.empty()) notes += ", "; notes += text; };
	if (m_hsc_save && m_savekey)
		note("saves to the High Score Cart or a SaveKey in port 2");
	else if (m_hsc_save)
		note("saves to the High Score Cart");
	else if (m_savekey)
		note("saves to a SaveKey in port 2");
	if (m_expansion)
		note("uses the XBoarD or XM");
	if (!notes.empty())
		details += "\n(" + notes + ")";
	return details;
}


/*-------------------------------------------------
 call_unload
 -------------------------------------------------*/

void a78_cart_slot_device::call_unload()
{
	if (m_cart && m_cart->get_nvram_base() && m_cart->get_nvram_size())
		battery_save(m_cart->get_nvram_base(), 0x800);
}



/*-------------------------------------------------
 verify_header - check the image (from fullpath)
 has an admissible header
 -------------------------------------------------*/

std::pair<std::error_condition, std::string> a78_cart_slot_device::verify_header(const uint8_t *header)
{
	const char *magic = "ATARI7800";

	if (strncmp(magic, reinterpret_cast<const char *>(header + 1), 9))
		return std::make_pair(image_error::INVALIDIMAGE, "Not a valid A7800 image");

	logerror("returning ID_OK\n");
	return std::make_pair(std::error_condition(), std::string());
}


/*-------------------------------------------------
 get default card software
 -------------------------------------------------*/

std::string a78_cart_slot_device::get_default_card_software(get_default_card_software_hook &hook) const
{
	if (hook.image_file())
	{
		uint64_t len;
		hook.image_file()->length(len); // FIXME: check error return

		// Load and check the header
		uint8_t head[128] = { };
		/*auto const [err, actual] =*/ read(*hook.image_file(), &head[0], 128); // FIXME: check error return or read returning short

		bool const has_header = !memcmp(&head[1], "ATARI7800", 9);
		int headerless = -1;
		if (!has_header)
		{
			// no header: a dump of the cartridge's ROM alone
			std::vector<uint8_t> data(len);
			hook.image_file()->seek(0, SEEK_SET);
			read(*hook.image_file(), &data[0], len);
			headerless = headerless_type(&data[0], len);
		}

		if (m_vcs_image)
		{
			// A 2600 cartridge goes in the 2600 cartridge slot, as if it had been given there, so
			// this slot takes 2600 images too (from the command line or the File Manager); it
			// keeps the image itself, with no board, so that this is done again whenever the
			// options are (a reset clears the option of an image that is not loaded yet).
			emu_options &options = mconfig().options();
			std::string const path(options.image_option(instance_name()).value());
			::image_option &vcs = options.image_option(m_vcs_image);
			if (!has_header && headerless == HEADERLESS_2600)
			{
				if (vcs.value().empty() || vcs.value() == s_vcs_forwarded)
				{
					s_vcs_forwarded = path;
					vcs.specify(path);
				}
			}
			else if (!s_vcs_forwarded.empty() && vcs.value() == s_vcs_forwarded)
			{
				// a 7800 cartridge now: take out the 2600 one this slot put in
				s_vcs_forwarded.clear();
				vcs.specify("");
			}
		}

		if (!has_header)
			return (headerless >= 0) ? a78_get_slot(headerless) : std::string();

		// the High Score Cartridge's own image
		if (len == 0x1000 + 128)
		{
			std::vector<uint8_t> data(0x1000);
			read(*hook.image_file(), &data[0], 0x1000);
			if (is_hsc_image(&data[0], 0x1000))
				return a78_get_slot(A78_HSC);
		}

		// let's try to auto-fix some common errors in the header
		int const mapper = validate_header(get_u16be(&head[53]), false);

		int type = A78_TYPE0;
		switch (mapper & 0x2e)
		{
			case 0x0000:
				type = BIT(mapper, 0) ? A78_TYPE1 : A78_TYPE0;
				break;
			case 0x0002:
				type = BIT(mapper, 0) ? A78_TYPE3 : A78_TYPE2;
				break;
			case 0x0006:
				type = A78_TYPE6;
				break;
			case 0x000a:
				type = A78_TYPEA;
				break;
			case 0x0022:
			case 0x0026:
				if (len > 0x40000)
					type = A78_MEGACART;
				else
					type = A78_VERSABOARD;
				break;
		}

		// check if cart has a POKEY at $0450 (typically a VersaBoard variant)!
		if (mapper & 0x40)
		{
			if (type != A78_TYPE2)
			{
				type &= ~0x02;
				type += A78_POKEY0450;
			}
		}
		if (a78_dual_pokey(mapper, type))
			type += A78_POKEY0440;

		if (int const special = a78_special_type(mapper, len); special >= 0)
			type = special;

		// check special bits, which override the previous
		if ((mapper & 0xff00) == 0x0100)
			type = A78_ACTIVISION;
		else if ((mapper & 0xff00) == 0x0200)
			type = A78_ABSOLUTE;
		else if ((mapper & 0x0080) == 0x0080)
			type = A78_TYPE8;

		logerror("Cart type: %x\n", type);
		char const *const slot_string = a78_get_slot(type);

		return std::string(slot_string);
	}
	else
	{
		// no image: nor the 2600 one this slot put in
		if (m_vcs_image && !s_vcs_forwarded.empty())
		{
			::image_option &vcs = mconfig().options().image_option(m_vcs_image);
			if (vcs.value() == s_vcs_forwarded)
			{
				s_vcs_forwarded.clear();
				vcs.specify("");
			}
		}
		return software_get_default_slot("a78_rom");
	}
}


/*-------------------------------------------------
 read
 -------------------------------------------------*/

uint8_t a78_cart_slot_device::read_04xx(offs_t offset)
{
	if (m_cart)
		return m_cart->read_04xx(offset);
	else
		return 0xff;
}

uint8_t a78_cart_slot_device::read_08xx(offs_t offset)
{
	if (m_cart)
		return m_cart->read_08xx(offset);
	else
		return 0xff;
}

uint8_t a78_cart_slot_device::read_10xx(offs_t offset)
{
	if (m_cart)
		return m_cart->read_10xx(offset);
	else
		return 0xff;
}

uint8_t a78_cart_slot_device::read_30xx(offs_t offset)
{
	if (m_cart)
		return m_cart->read_30xx(offset);
	else
		return 0xff;
}

uint8_t a78_cart_slot_device::read_40xx(offs_t offset)
{
	if (m_cart)
		return m_cart->read_40xx(offset);
	else
		return 0xff;
}


/*-------------------------------------------------
 write
 -------------------------------------------------*/

void a78_cart_slot_device::write_04xx(offs_t offset, uint8_t data)
{
	if (m_cart)
		m_cart->write_04xx(offset, data);
}

void a78_cart_slot_device::write_08xx(offs_t offset, uint8_t data)
{
	if (m_cart)
		m_cart->write_08xx(offset, data);
}

void a78_cart_slot_device::write_10xx(offs_t offset, uint8_t data)
{
	if (m_cart)
		m_cart->write_10xx(offset, data);
}

void a78_cart_slot_device::write_30xx(offs_t offset, uint8_t data)
{
	if (m_cart)
		m_cart->write_30xx(offset, data);
}

void a78_cart_slot_device::write_40xx(offs_t offset, uint8_t data)
{
	if (m_cart)
		m_cart->write_40xx(offset, data);
}


/*-------------------------------------------------
 A78 header logging

 A78 HEADER FORMAT

 Bytes  | Content           | Length
 ========================================
 0      | Header version    |  1 byte
 -------|-------------------|------------
 1..16  | "ATARI7800     "  | 16 bytes
 -------|-------------------|------------
 17..48 | Cart title        | 32 bytes
 -------|-------------------|------------
 49..52 | Data length       |  4 bytes
 -------|-------------------|------------
 53..54 | Cart type [*]     |  2 bytes
 -------|-------------------|------------
 55     | Controller 1 type |  1 byte
        |                   |
        | 0 = None          |
        | 1 = Joystick      |
        | 2 = Light Gun     |
 -------|-------------------|------------
 56     | Controller 2 type |  1 byte
        |                   |
        | As above          |
 -------|-------------------|------------
 57     | TV System         |  1 byte
        |                   |
        | 0 = NTSC/1 = PAL  |
 -------|-------------------|------------
 58     | Save data         |  1 byte
        |                   |  (only v2)
        | 0 = None / Unk    |
        | 1 = High Score    |
        | 2 = Savekey       |
 -------|-------------------|-----------
 63     | Expansion module  |  1 byte
        |                   |
        | 0 = No expansion  |
        |     module        |
        | 1 = Expansion     |
        |     required      |
 -------|-------------------|-----------


 [*] Cart type:

 (byte 54)
 bit 0-7 - Hardware "flags"
 bit 0 [0x01] - POKEY at $4000
 bit 1 [0x02] - SuperCart bank switched
 bit 2 [0x04] - SuperCart RAM at $4000
 bit 3 [0x08] - bank 0 of 144K ROM at $4000
 bit 4 [0x10] - bank 6 at $4000
 bit 5 [0x20] - banked RAM at $4000
 bit 6 [0x40] - POKEY at $0450
 bit 7 [0x80] - Mirror RAM at $4000

 (byte 53)
 bit0 set = Absolute mapper (F18 Hornet)
 bit1 set = Activision mapper (Double Dragon & Rampage)

 -------------------------------------------------*/

void a78_cart_slot_device::internal_header_logging(const uint8_t *header, uint32_t len)
{
	char head_title[35];
	uint32_t head_length = get_u32be(&header[49]);
	uint16_t head_mapper = get_u16be(&header[53]);
	uint8_t head_ctrl1 = header[55];
	uint8_t head_ctrl2 = header[56];
	uint8_t head_ispal = header[57];
	std::string cart_mapper, ctrl1, ctrl2;
	memcpy(head_title, header + 0x11, 0x20);

	switch (head_mapper)
	{
		case 0x0000:
			cart_mapper.assign("No Bankswitch");
			break;
		case 0x0001:
			cart_mapper.assign("No Bankswitch + POKEY");
			break;
		case 0x0002:
			cart_mapper.assign("SuperCart Bankswitch");
			break;
		case 0x0003:
			cart_mapper.assign("SuperCart Bankswitch + POKEY");
			break;
		case 0x0006:
			cart_mapper.assign("SuperCart Bankswitch + RAM");
			break;
		case 0x000a:
			cart_mapper.assign("SuperCart 9Banks");
			break;
		case 0x000b:
			cart_mapper.assign("SuperCart XM Compatible");
			break;
		case 0x0020:
			cart_mapper.assign("SuperCart Bankswitch + 32K RAM");
			break;
		case 0x0080:
			cart_mapper.assign("No Bankswitch + Mirror RAM");
			break;
		case 0x0100:
			cart_mapper.assign("Activision Bankswitch");
			break;
		case 0x0200:
			cart_mapper.assign("Absolute Bankswitch");
			break;
		default:
			cart_mapper.assign("Unknown mapper");
			break;
	}

	switch (head_ctrl1)
	{
		case 0x00:
			ctrl1.assign("None");
			break;
		case 0x01:
			ctrl1.assign("Joystick");
			break;
		case 0x02:
			ctrl1.assign("Light Gun");
			break;
		default:
			ctrl1.assign("Unknown controller");
			break;
	}

	switch (head_ctrl2)
	{
		case 0x00:
			ctrl2.assign("None");
			break;
		case 0x01:
			ctrl2.assign("Joystick");
			break;
		case 0x02:
			ctrl2.assign("Light Gun");
			break;
		default:
			ctrl2.assign("Unknown controller");
			break;
	}

	logerror( "ROM DETAILS\n" );
	logerror( "===========\n\n" );
	logerror( "\tTotal length (with header):  0x%x (%dK + 128b header)\n\n", len, len/0x400);
	logerror( "HEADER DETAILS\n" );
	logerror( "==============\n\n" );
	logerror( "\tTitle:           %.32s\n", head_title);
	logerror( "\tLength:          0x%X [real 0x%X]\n", head_length, len);
	logerror( "\tMapper:          %s [0x%X]\n", cart_mapper, head_mapper);
	logerror( "\t\tPOKEY:           %s\n", BIT(head_mapper, 0) ? "Yes" : "No");
	logerror( "\t\tSC Bankswitch:   %s\n", BIT(head_mapper, 1) ? "Yes" : "No");
	logerror( "\t\tRAM at $4000:    %s\n", BIT(head_mapper, 2) ? "Yes" : "No");
	logerror( "\t\tbank0 at $4000:  %s\n", BIT(head_mapper, 3) ? "Yes" : "No");
	logerror( "\t\tbank6 at $4000:  %s\n", BIT(head_mapper, 4) ? "Yes" : "No");
	logerror( "\t\tbanked RAM:      %s\n", BIT(head_mapper, 5) ? "Yes" : "No");
	logerror( "\t\tPOKEY at $450:   %s\n", BIT(head_mapper, 6) ? "Yes" : "No");
	logerror( "\t\tmRAM at $4000:   %s\n", BIT(head_mapper, 7) ? "Yes" : "No");
	logerror( "\t\tSpecial:         %s ", (head_mapper & 0xff00) ? "Yes" : "No");
	if (head_mapper & 0xff00)
	{
		logerror( "[%s]\n", (head_mapper & 0xff00) == 0x100 ? "Absolute" :
								(head_mapper & 0xff00) == 0x200 ? "Activision" : "Unknown" );
	}
	else
		logerror( "\n");
	logerror( "\tController 1:    0x%.2X [%s]\n", head_ctrl1, ctrl1);
	logerror( "\tController 2:    0x%.2X [%s]\n", head_ctrl2, ctrl2);
	logerror( "\tVideo:           %s\n", (head_ispal) ? "PAL" : "NTSC");
}
