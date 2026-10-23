// license:BSD-3-Clause
// copyright-holders:Dan Boris
#ifndef MAME_ATARI_MARIA_H
#define MAME_ATARI_MARIA_H

#pragma once


// ======================> atari_maria_device

class atari_maria_device :  public device_t, public device_video_interface, public device_memory_interface
{
public:
	// construction/destruction
	atari_maria_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

	template <typename T> void set_dmaspace_tag(T &&tag, int spacenum) { m_dmaspace.set_tag(std::forward<T>(tag), spacenum); }

	// MARIA's own view of the bus, "dma" (address space 0): when the driver gives it a map and
	// points the DMA at it (set_dmaspace_tag(*this, 0)), MARIA's fetches are its own memory
	// accesses -- a watchpoint or Lua tap on the CPU's space no longer sees them, and one on this
	// space sees only them
	static constexpr int AS_DMA = 0;

	auto dma_wait_callback() { return m_dma_wait_cb.bind(); }
	auto halt_callback() { return m_halt_cb.bind(); }
	auto dli_callback() { return m_dli_cb.bind(); }

	void interrupt(int lines);
	void startdma(int lines);
	void display_visible(int pixelx);

	// the CPU address range of a bankset cartridge (its ROM and RAM): display lists fetched from it
	// read as empty (see read_dl); they belong in console RAM
	void set_cart_ram_window(offs_t lo, offs_t hi) { m_cart_ram_lo = lo; m_cart_ram_hi = hi; m_dl_warned = false; }

	uint8_t read(offs_t offset);
	void write(offs_t offset, uint8_t data);

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

protected:
	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual space_config_vector memory_space_config() const override;
	virtual void device_reset() override ATTR_COLD;

private:
	static constexpr uint32_t LINERAM_SIZE = 160;

	void draw_scanline();
	bool is_holey(offs_t addr);
	int write_line_ram(offs_t addr, uint8_t offset, uint8_t pal);

	uint8_t read_byte(offs_t addr) { return m_dmaspace->read_byte(addr); }
	uint8_t read_dl(offs_t addr);

	uint8_t m_maria_palette[32];
	std::unique_ptr<uint8_t []> m_line_ram[2];
	uint8_t m_active_buffer;
	bool m_write_mode;
	uint32_t m_dll;
	uint32_t m_dl;
	uint8_t m_holey;
	int32_t m_offset;
	uint8_t m_vblank;
	bool m_dmaon;
	uint16_t m_dpp;
	bool m_wsync;
	bool m_color_kill;
	bool m_cwidth;
	bool m_bcntl;
	bool m_kangaroo;
	uint8_t m_rm;
	bool m_dli;
	bool m_first_dma = false;   // the next line drawn is the frame's first DMA (it costs more)
	uint32_t m_charbase;
	bitmap_ind16 m_bitmap;

	address_space_config m_dma_config;
	offs_t m_cart_ram_lo = 1;
	offs_t m_cart_ram_hi = 0;      // an empty window until the driver sets one
	bool m_dl_warned = false;

	required_address_space m_dmaspace;

	devcb_write64 m_dma_wait_cb;
	devcb_write_line m_halt_cb;
	devcb_write_line m_dli_cb;
};


// device type definition
DECLARE_DEVICE_TYPE(ATARI_MARIA, atari_maria_device)

#endif // MAME_ATARI_MARIA_H
