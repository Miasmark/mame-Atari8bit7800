// license:BSD-3-Clause
// copyright-holders:Fabio Priuli, Angelo Salese
/**************************************************************************************************

Atari A800/A5200/XEGS cart slot emulation

References:
- Altirra HW reference manual 2022-07-07 edition, chapter 8;
- https://github.com/atari800/atari800/blob/master/DOC/cart.txt
- https://www.atarimax.com/jindroush.atari.org/acarts.html

**************************************************************************************************/

#include "emu.h"
#include "a800_slot.h"

#include "hashfile.h"
#include "multibyte.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(A800_CART_SLOT,  a800_cart_slot_device,  "a800_cart_slot",  "Atari 8-bit Cartridge Slot")
DEFINE_DEVICE_TYPE(A5200_CART_SLOT, a5200_cart_slot_device, "a5200_cart_slot", "Atari 5200 Cartridge Slot")


//-------------------------------------------------
//  device_a800_cart_interface - constructor
//-------------------------------------------------

device_a800_cart_interface::device_a800_cart_interface (const machine_config &mconfig, device_t &device)
	: device_interface(device, "a800cart")
	, m_rom(nullptr)
	, m_rom_size(0)
	, m_bank_mask(0)
{
	m_slot = dynamic_cast<a800_cart_slot_device *>(device.owner());
}

device_a5200_cart_interface::device_a5200_cart_interface (const machine_config &mconfig, device_t &device)
	: device_interface(device, "a5200cart")
	, m_rom(nullptr)
	, m_rom_size(0)
	, m_bank_mask(0)
{
	m_slot = dynamic_cast<a5200_cart_slot_device *>(device.owner());
}


//-------------------------------------------------
//  ~device_a800_cart_interface  - destructor
//-------------------------------------------------

device_a800_cart_interface::~device_a800_cart_interface ()
{
}

device_a5200_cart_interface::~device_a5200_cart_interface ()
{
}

//-------------------------------------------------
//  rom_alloc - alloc the space for the cart
//-------------------------------------------------

void device_a800_cart_interface::rom_alloc(uint32_t size)
{
	if (m_rom == nullptr)
	{
		// TODO: shouldn't really load from fixed tag
		// (particularly inconvenient for stuff like flash ROM hookups)
		m_rom = device().machine().memory().region_alloc(device().subtag("^cart:rom"), size, 1, ENDIANNESS_LITTLE)->base();
		m_rom_size = size;

		// setup other helpers
		// TODO: unusable for SIC! and any other mapping that maps over 0x4000 rather than 0x2000
		m_bank_mask = (size / 0x2000) - 1;  // code for XEGS carts makes use of this to simplify banking
	}
}

void device_a800_cart_interface::interface_pre_start()
{
	if (!m_slot->started())
		throw device_missing_dependencies();
}

void device_a800_cart_interface::interface_post_start()
{
	m_slot->m_space_mem->install_device(0x0000, 0x3fff, *this, &device_a800_cart_interface::cart_map);
	m_slot->m_space_io->install_device(0x0000, 0x00ff, *this, &device_a800_cart_interface::cctl_map);
}

void device_a800_cart_interface::cart_map(address_map &map)
{
	map(0x0000, 0x3fff).unmaprw();
}

void device_a800_cart_interface::cctl_map(address_map &map)
{
	map(0x0000, 0x00ff).unmaprw();
}

void device_a800_cart_interface::rd4_w(int state) { m_slot->m_rd4_cb(state); }
void device_a800_cart_interface::rd5_w(int state) { m_slot->m_rd5_cb(state); }
// helper to call both lines at same time, for anything banking on the full range.
void device_a800_cart_interface::rd_both_w(int state) { rd4_w(state); rd5_w(state); }


// a5200

void device_a5200_cart_interface::rom_alloc(uint32_t size)
{
	if (m_rom == nullptr)
	{
		// TODO: shouldn't really load from fixed tag
		// (particularly inconvenient for stuff like flash ROM hookups)
		m_rom = device().machine().memory().region_alloc(device().subtag("^cart:rom"), size, 1, ENDIANNESS_LITTLE)->base();
		m_rom_size = size;

		// setup other helpers
		// TODO: unusable for SIC! and any other mapping that maps over 0x4000 rather than 0x2000
		m_bank_mask = (size / 0x2000) - 1;  // code for XEGS carts makes use of this to simplify banking
	}
}

void device_a5200_cart_interface::cart_map(address_map &map)
{
	map(0x0000, 0x7fff).unmaprw();
}

void device_a5200_cart_interface::interface_pre_start()
{
	if (!m_slot->started())
		throw device_missing_dependencies();
}

void device_a5200_cart_interface::interface_post_start()
{
	m_slot->m_space_mem->install_device(0x0000, 0x7fff, *this, &device_a5200_cart_interface::cart_map);
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  ****_cart_slot_device - constructor
//-------------------------------------------------

a800_cart_slot_device::a800_cart_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, A800_CART_SLOT, tag, owner, clock)
	, device_memory_interface(mconfig, *this)
	, device_cartrom_image_interface(mconfig, *this)
	, device_single_card_slot_interface<device_a800_cart_interface>(mconfig, *this)
	, m_cart(nullptr)
	, m_rd4_cb(*this)
	, m_rd5_cb(*this)
	, m_space_mem_config("cart_mem", ENDIANNESS_LITTLE, 8, 14, 0, address_map_constructor())
	, m_space_io_config("cart_io", ENDIANNESS_LITTLE, 8, 8, 0, address_map_constructor())
	, m_is_xegs(false)
{
}

a5200_cart_slot_device::a5200_cart_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, A5200_CART_SLOT, tag, owner, clock)
	, device_memory_interface(mconfig, *this)
	, device_cartrom_image_interface(mconfig, *this)
	, device_single_card_slot_interface<device_a5200_cart_interface>(mconfig, *this)
	, m_cart(nullptr)
	, m_space_mem_config("cart_mem", ENDIANNESS_LITTLE, 8, 15, 0, address_map_constructor())
{
}


//-------------------------------------------------
//  ****_cart_slot_device - destructor
//-------------------------------------------------

a800_cart_slot_device::~a800_cart_slot_device()
{
}

a5200_cart_slot_device::~a5200_cart_slot_device()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a800_cart_slot_device::device_start()
{
	m_cart = get_card_device();
	m_space_io = &space(AS_IO);
	m_space_mem = &space(AS_PROGRAM);
}

void a5200_cart_slot_device::device_start()
{
	m_cart = get_card_device();
	m_space_mem = &space(AS_PROGRAM);
}

/*-------------------------------------------------
 call load
 -------------------------------------------------*/

//-------------------------------------------------
//  A800 PCBs
//-------------------------------------------------

struct a800_slot
{
	int                     pcb_id;
	const char              *slot_option;
};

// Here, we take the feature attribute from .xml (i.e. the PCB name) and we assign a unique ID to it
static const a800_slot slot_list[] =
{
	{ A800_8K,        "a800_8k" },
	{ A800_16K,       "a800_16k" },
	{ A800_OSS034M,   "a800_oss034m" },
	{ A800_OSS043M,   "a800_oss043m" },
	{ A800_OSSM091,   "a800_ossm091" },
	{ A800_OSS8K,     "a800_oss8k" },
	{ A800_PHOENIX,   "a800_phoenix" },
	{ A800_BLIZZARD,  "a800_blizzard" },
	{ A800_XEGS,      "xegs" },
	{ A800_BBSB,      "a800_bbsb" },
	{ A800_DIAMOND,   "a800_diamond" },
	{ A800_WILLIAMS,  "a800_williams" },
	{ A800_EXPRESS,   "a800_express" },
	{ A800_SPARTADOS, "a800_sparta" },
	{ A800_SPARTADOS_128KB, "a800_sparta_128kb" },
	{ A800_TURBO,     "a800_turbo" },
	{ A800_BLIZZARD_32KB, "a800_blizzard_32kb" },
	{ A800_TELELINK2, "a800_tlink2" },
	{ A800_ULTRACART, "a800_ultracart" },
	{ A800_ATRAX,     "a800_atrax" },
	{ A800_CORINA,    "a800_corina" },
	{ A800_CORINA_SRAM, "a800_corina_sram" },
	{ SIC_128KB,      "sic_128kb" },
	{ SIC_256KB,      "sic_256kb" },
	{ SIC_512KB,      "sic_512kb" },
	{ ATARIMAX_MAXFLASH_128KB, "maxflash_128kb" },
	{ ATARIMAX_MAXFLASH_1MB, "maxflash_1mb" },
	{ A800_ADAWLIAH,  "a800_adawliah" },
	{ A800_8K_RIGHT,  "a800_8k_right" },
	{ A5200_4K,       "a5200_rom" },
	{ A5200_8K,       "a5200_rom" },
	{ A5200_16K,      "a5200_rom" },
	{ A5200_32K,      "a5200_rom" },
	{ A5200_16K_2CHIPS, "a5200_2chips" },
	{ A5200_BBSB,     "a5200_bbsb" }
};


static int a800_get_pcb_id(const char *slot)
{
	for (auto & elem : slot_list)
	{
		if (!strcmp(elem.slot_option, slot))
			return elem.pcb_id;
	}

	return 0;
}

static const char *a800_get_slot(int type)
{
	for (auto & elem : slot_list)
	{
		if (elem.pcb_id == type)
			return elem.slot_option;
	}

	return "a800_8k";
}


//-------------------------------------------------
//  CART headers (.car)
//
//  The Atari800 emulator's format: "CART", a type, a checksum of the data (the sum of its
//  bytes), four unused bytes, then the ROM. The types are listed in its DOC/cart.txt; the table
//  below is that list, so a header MAME cannot run is named rather than numbered.
//-------------------------------------------------

namespace {

struct car_type
{
	uint32_t id;
	bool a5200;
	uint32_t size_kb;
	const char *name;
};

constexpr car_type CAR_TYPES[] =
{
	{  1, false,     8, "Standard 8 KB cartridge" },
	{  2, false,    16, "Standard 16 KB cartridge" },
	{  3, false,    16, "OSS two chip 16 KB cartridge (034M)" },
	{  4, true,     32, "Standard 32 KB 5200 cartridge" },
	{  5, false,    32, "DB 32 KB cartridge" },
	{  6, true,     16, "Two chip 16 KB 5200 cartridge" },
	{  7, true,     40, "Bounty Bob Strikes Back 40 KB 5200 cartridge" },
	{  8, false,    64, "64 KB Williams cartridge" },
	{  9, false,    64, "Express 64 KB cartridge" },
	{ 10, false,    64, "Diamond 64 KB cartridge" },
	{ 11, false,    64, "SpartaDOS X 64 KB cartridge" },
	{ 12, false,    32, "XEGS 32 KB cartridge" },
	{ 13, false,    64, "XEGS 64 KB cartridge" },
	{ 14, false,   128, "XEGS 128 KB cartridge" },
	{ 15, false,    16, "OSS one chip 16 KB cartridge" },
	{ 16, true,     16, "One chip 16 KB 5200 cartridge" },
	{ 17, false,   128, "Atrax 128 KB cartridge" },
	{ 18, false,    40, "Bounty Bob Strikes Back 40 KB cartridge" },
	{ 19, true,      8, "Standard 8 KB 5200 cartridge" },
	{ 20, true,      4, "Standard 4 KB 5200 cartridge" },
	{ 21, false,     8, "Right slot 8 KB cartridge" },
	{ 22, false,    32, "32 KB Williams cartridge" },
	{ 23, false,   256, "XEGS 256 KB cartridge" },
	{ 24, false,   512, "XEGS 512 KB cartridge" },
	{ 25, false,  1024, "XEGS 1 MB cartridge" },
	{ 26, false,    16, "MegaCart 16 KB cartridge" },
	{ 27, false,    32, "MegaCart 32 KB cartridge" },
	{ 28, false,    64, "MegaCart 64 KB cartridge" },
	{ 29, false,   128, "MegaCart 128 KB cartridge" },
	{ 30, false,   256, "MegaCart 256 KB cartridge" },
	{ 31, false,   512, "MegaCart 512 KB cartridge" },
	{ 32, false,  1024, "MegaCart 1 MB cartridge" },
	{ 33, false,    32, "Switchable XEGS 32 KB cartridge" },
	{ 34, false,    64, "Switchable XEGS 64 KB cartridge" },
	{ 35, false,   128, "Switchable XEGS 128 KB cartridge" },
	{ 36, false,   256, "Switchable XEGS 256 KB cartridge" },
	{ 37, false,   512, "Switchable XEGS 512 KB cartridge" },
	{ 38, false,  1024, "Switchable XEGS 1 MB cartridge" },
	{ 39, false,     8, "Phoenix 8 KB cartridge" },
	{ 40, false,    16, "Blizzard 16 KB cartridge" },
	{ 41, false,   128, "Atarimax 128 KB Flash cartridge" },
	{ 42, false,  1024, "Atarimax 1 MB Flash cartridge" },
	{ 43, false,   128, "SpartaDOS X 128 KB cartridge" },
	{ 44, false,     8, "OSS 8 KB cartridge" },
	{ 45, false,    16, "OSS two chip 16 KB cartridge (043M)" },
	{ 46, false,     4, "Blizzard 4 KB cartridge" },
	{ 47, false,    32, "AST 32 KB cartridge" },
	{ 48, false,    64, "Atrax SDX 64 KB cartridge" },
	{ 49, false,   128, "Atrax SDX 128 KB cartridge" },
	{ 50, false,    64, "Turbosoft 64 KB cartridge" },
	{ 51, false,   128, "Turbosoft 128 KB cartridge" },
	{ 52, false,    32, "Ultracart 32 KB cartridge" },
	{ 53, false,     8, "Low bank 8 KB cartridge" },
	{ 54, false,   128, "SIC! 128 KB cartridge" },
	{ 55, false,   256, "SIC! 256 KB cartridge" },
	{ 56, false,   512, "SIC! 512 KB cartridge" },
	{ 57, false,     2, "Standard 2 KB cartridge" },
	{ 58, false,     4, "Standard 4 KB cartridge" },
	{ 59, false,     4, "Right slot 4 KB cartridge" },
	{ 60, false,    32, "Blizzard 32 KB cartridge" },
};

const car_type *find_car_type(uint32_t id)
{
	for (auto const &t : CAR_TYPES)
		if (t.id == id)
			return &t;
	return nullptr;
}

// MAME's PCB for a CART type, or -1 when MAME has none
int car_pcb(uint32_t id)
{
	switch (id)
	{
	case 1: case 57: case 58:   return A800_8K;            // 2 KB and 4 KB mirror through the 8 KB window
	case 2:                     return A800_16K;
	case 3:                     return A800_OSS034M;
	case 4:                     return A5200_32K;
	case 6:                     return A5200_16K_2CHIPS;
	case 7:                     return A5200_BBSB;
	case 8: case 22:            return A800_WILLIAMS;      // 64 KB and 32 KB: the bank mask follows the size
	case 9:                     return A800_EXPRESS;       // $D57x
	case 10:                    return A800_DIAMOND;       // $D5Dx
	case 11:                    return A800_SPARTADOS;
	case 12: case 13: case 14: case 23: case 24: case 25:
	case 33: case 34: case 35: case 36: case 37: case 38:
	                            return A800_XEGS;          // the bank mask follows the size
	case 15:                    return A800_OSSM091;
	case 16:                    return A5200_16K;
	case 17:                    return A800_ATRAX;
	case 18:                    return A800_BBSB;
	case 19:                    return A5200_8K;
	case 20:                    return A5200_4K;
	case 21: case 59:           return A800_8K_RIGHT;
	case 39:                    return A800_PHOENIX;
	case 40:                    return A800_BLIZZARD;
	case 41:                    return ATARIMAX_MAXFLASH_128KB;
	case 42:                    return ATARIMAX_MAXFLASH_1MB;
	case 43:                    return A800_SPARTADOS_128KB;
	case 44:                    return A800_OSS8K;
	case 45:                    return A800_OSS043M;
	case 50: case 51:           return A800_TURBO;
	case 52:                    return A800_ULTRACART;
	case 54:                    return SIC_128KB;
	case 55:                    return SIC_256KB;
	case 56:                    return SIC_512KB;
	case 60:                    return A800_BLIZZARD_32KB;
	case 69:                    return A800_ADAWLIAH;
	default:                    return -1;
	}
}

bool is_car_header(const uint8_t *header, uint64_t len)
{
	return len >= 0x10 && !memcmp(header, "CART", 4);
}

// say something about a header once, though MAME asks for the slot more than once
void car_warn(std::string &last, std::string const &text)
{
	if (text != last)
		osd_printf_warning("%s\n", text);
	last = text;
}

// the PCB for a header; with `warned` (the last warning given), it says what does not fit (when the
// slot is chosen)
int identify_car(const uint8_t *header, uint64_t datalen, std::string *warned)
{
	uint32_t const id = get_u32be(&header[4]);
	car_type const *const t = find_car_type(id);
	std::string const name = t ? util::string_format("%s (CART type %u)", t->name, id) : util::string_format("CART type %u", id);
	int pcb = car_pcb(id);
	if (pcb < 0)
	{
		// not emulated: the plain cartridge its size would be, which at least shows the first bank
		pcb = (t && t->a5200) ? (datalen > 0x4000 ? A5200_32K : datalen > 0x2000 ? A5200_16K : datalen > 0x1000 ? A5200_8K : A5200_4K)
				: (datalen > 0x2000 ? A800_16K : A800_8K);
		if (warned)
			car_warn(*warned, util::string_format("%s is not emulated by MAME; loading it as MAME's %s, which will not run it properly",
					name, a800_get_slot(pcb)));
	}
	else if (warned && id >= 33 && id <= 38)
		car_warn(*warned, util::string_format("%s: loaded as XEGS; the switch that turns the cartridge off is not emulated", name));
	if (warned && t && uint64_t(t->size_kb) * 1024 != datalen)
		car_warn(*warned, util::string_format("%s should hold %u KB, but the file has %u bytes after its header", name, t->size_kb, uint32_t(datalen)));
	return pcb;
}

} // anonymous namespace

std::pair<std::error_condition, std::string> a800_cart_slot_device::call_load()
{
	if (m_cart)
	{
		uint32_t len;

		if (loaded_through_softlist())
		{
			const char *pcb_name;
			len = get_software_region_length("rom");

			m_cart->rom_alloc(len);
			memcpy(m_cart->get_rom_base(), get_software_region("rom"), len);

			if ((pcb_name = get_feature("slot")) != nullptr)
				m_type = a800_get_pcb_id(pcb_name);
			else
				m_type = A800_8K;
		}
		else
		{
			len = length();

			// a CART header names the cart type (see identify_car)
			uint8_t header[16] = { 0 };
			bool const has_header = len >= 0x10 && fread(header, 0x10) == 0x10 && is_car_header(header, len);
			if (has_header)
			{
				m_type = identify_car(header, len - 0x10, nullptr);
				len -= 0x10;
			}
			else    // otherwise try to guess based on size
			{
				if (len == 0x8000)
					m_type = A5200_32K;
				if (len == 0x4000)
					m_type = A800_16K;
				if (len == 0x2000)
					m_type = A800_8K;
				if (len == 0x1000)
					m_type = A5200_4K;
				// also make a try with .hsi file (for .a52 files)
				std::string info;
				if (hashfile_extrainfo(*this, info) && info.compare("A13MIRRORING")==0)
					m_type = A5200_16K_2CHIPS;
			}

			if (!has_header)
				fseek(0, SEEK_SET);
			m_cart->rom_alloc(len);
			fread(m_cart->get_rom_base(), len);
			if (has_header)
			{
				uint32_t sum = 0;
				for (uint32_t i = 0; i < len; i++)
					sum += m_cart->get_rom_base()[i];
				if (sum != get_u32be(&header[8]))
					osd_printf_warning("CART header checksum is %08X, the data sums to %08X: the image may be damaged\n", get_u32be(&header[8]), sum);
			}
		}

		logerror("%s loaded cartridge '%s' size %dK\n", machine().system().name, filename(), len/1024);
	}
	return std::make_pair(std::error_condition(), std::string());
}

std::pair<std::error_condition, std::string> a5200_cart_slot_device::call_load()
{
	if (m_cart)
	{
		uint32_t len;

		if (loaded_through_softlist())
		{
			const char *pcb_name;
			len = get_software_region_length("rom");

			m_cart->rom_alloc(len);
			memcpy(m_cart->get_rom_base(), get_software_region("rom"), len);

			if ((pcb_name = get_feature("slot")) != nullptr)
				m_type = a800_get_pcb_id(pcb_name);
			else
				m_type = A800_8K;
		}
		else
		{
			len = length();

			// a CART header names the cart type (see identify_car)
			uint8_t header[16] = { 0 };
			bool const has_header = len >= 0x10 && fread(header, 0x10) == 0x10 && is_car_header(header, len);
			if (has_header)
			{
				m_type = identify_car(header, len - 0x10, nullptr);
				len -= 0x10;
			}
			else    // otherwise try to guess based on size
			{
				if (len == 0x8000)
					m_type = A5200_32K;
				if (len == 0x4000)
					m_type = A800_16K;
				if (len == 0x2000)
					m_type = A800_8K;
				if (len == 0x1000)
					m_type = A5200_4K;
				// also make a try with .hsi file (for .a52 files)
				std::string info;
				if (hashfile_extrainfo(*this, info) && info.compare("A13MIRRORING")==0)
					m_type = A5200_16K_2CHIPS;
			}

			if (!has_header)
				fseek(0, SEEK_SET);
			m_cart->rom_alloc(len);
			fread(m_cart->get_rom_base(), len);
			if (has_header)
			{
				uint32_t sum = 0;
				for (uint32_t i = 0; i < len; i++)
					sum += m_cart->get_rom_base()[i];
				if (sum != get_u32be(&header[8]))
					osd_printf_warning("CART header checksum is %08X, the data sums to %08X: the image may be damaged\n", get_u32be(&header[8]), sum);
			}
		}

		logerror("%s loaded cartridge '%s' size %dK\n", machine().system().name, filename(), len/1024);
	}
	return std::make_pair(std::error_condition(), std::string());
}

void a800_cart_slot_device::call_unload()
{
}

void a5200_cart_slot_device::call_unload()
{
}


/*-------------------------------------------------
 get default card software
 -------------------------------------------------*/

std::string a800_cart_slot_device::get_default_card_software(get_default_card_software_hook &hook) const
{
	// Nope, will crash when mounting the SDX subslot
//  std::string slot_default_option = default_option();
	const bool is_xegs = m_is_xegs;

	if (hook.image_file())
	{
		uint64_t len;
		hook.image_file()->length(len); // FIXME: check error return

		int type = is_xegs ? A800_XEGS : A800_8K;
		// a CART header names the cart type
		uint8_t head[0x10] = { 0 };
		if (len >= 0x10)
			/*auto const [err, actual] =*/ read(*hook.image_file(), &head[0], 0x10); // FIXME: check error return or read returning short
		if (is_car_header(head, len))
		{
			type = identify_car(&head[0], len - 0x10, &m_car_warned);
		}
		else    // otherwise try to guess based on size
		{
			if (is_xegs)
				return std::string("xegs");
			// TODO: very incomplete, is it also worth it?
			// Altirra sports a very complex scoring analysis (including code binary patterns),
			// and it still grants to the user multiple options
			if (len == 0x4000)
				type = A800_16K;
			if (len == 0x2000)
				type = A800_8K;
		}

		if (is_xegs && type != A800_XEGS)
		{
			osd_printf_info("This game is not designed for XEGS.\n");
			osd_printf_info("You might want to run it in %s.\n", type >= A5200_4K ? "A5200" : "A800");
		}
		else if (type >= A5200_4K)
			osd_printf_info("This game is not designed for A800. You might want to run it in A5200.\n");

		char const *const slot_string = a800_get_slot(type);

		return std::string(slot_string);
	}

	return software_get_default_slot(is_xegs ? "xegs" : "a800_8k");
}


std::string a5200_cart_slot_device::get_default_card_software(get_default_card_software_hook &hook) const
{
	if (hook.image_file())
	{
		uint64_t len;
		hook.image_file()->length(len); // FIXME: check error return

		// a CART header names the cart type
		int type = A5200_8K;
		uint8_t head[0x10] = { 0 };
		if (len >= 0x10)
			/*auto const [err, actual] =*/ read(*hook.image_file(), &head[0], 0x10); // FIXME: check error return or read returning short
		if (is_car_header(head, len))
		{
			type = identify_car(&head[0], len - 0x10, &m_car_warned);
		}
		else
		{
			std::string info;
			if (hook.hashfile_extrainfo(info) && info.compare("A13MIRRORING")==0)
				type = A5200_16K_2CHIPS;
		}
		if (type < A5200_4K)
			osd_printf_info("This game is not designed for A5200. You might want to run it in A800 or A800XL.\n");

		char const *const slot_string = a800_get_slot(type);

		return std::string(slot_string);
	}

	return software_get_default_slot("a5200_rom");
}


/*-------------------------------------------------
 read
 -------------------------------------------------*/

device_memory_interface::space_config_vector a800_cart_slot_device::memory_space_config() const
{
	return space_config_vector{
		std::make_pair(AS_PROGRAM, &m_space_mem_config),
		std::make_pair(AS_IO, &m_space_io_config)
	};
}

template <unsigned Bank> uint8_t a800_cart_slot_device::read_cart(offs_t offset)
{
	return m_space_mem->read_byte(offset | Bank << 13);
}

template <unsigned Bank> void a800_cart_slot_device::write_cart(offs_t offset, uint8_t data)
{
	m_space_mem->write_byte(offset | Bank << 13, data);
}

// Instantiate maps
template uint8_t a800_cart_slot_device::read_cart<0>(offs_t offset);
template uint8_t a800_cart_slot_device::read_cart<1>(offs_t offset);
template void a800_cart_slot_device::write_cart<0>(offs_t offset, uint8_t data);
template void a800_cart_slot_device::write_cart<1>(offs_t offset, uint8_t data);

uint8_t a800_cart_slot_device::read_cctl(offs_t offset)
{
	return m_space_io->read_byte(offset);
}

void a800_cart_slot_device::write_cctl(offs_t offset, uint8_t data)
{
	m_space_io->write_byte(offset, data);
}

device_memory_interface::space_config_vector a5200_cart_slot_device::memory_space_config() const
{
	return space_config_vector{
		std::make_pair(AS_PROGRAM, &m_space_mem_config)
	};
}

uint8_t a5200_cart_slot_device::read_cart(offs_t offset)
{
	return m_space_mem->read_byte(offset);
}

void a5200_cart_slot_device::write_cart(offs_t offset, uint8_t data)
{
	return m_space_mem->write_byte(offset, data);
}
