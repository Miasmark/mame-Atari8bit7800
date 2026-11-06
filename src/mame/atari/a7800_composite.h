// license:BSD-3-Clause
// copyright-holders:Jamie Blanks
/***************************************************************************

    Atari 7800 composite video: MARIA's colour and luma codes to NTSC
    composite and back, the way a television decodes it

    A C++ port, clock for clock and bit for bit, of composite_out.sv and
    composite_decoder.sv from MiSTer's Atari 7800 core (Jamie Blanks,
    2026), which carry this notice:

        SPDX-License-Identifier: MIT
        Copyright (c) 2026 Jamie Blanks

        Permission is hereby granted, free of charge, to any person
        obtaining a copy of this software and associated documentation
        files (the "Software"), to deal in the Software without
        restriction, including without limitation the rights to use,
        copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following
        conditions: The above copyright notice and this permission notice
        shall be included in all copies or substantial portions of the
        Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
        ANY KIND.

    MARIA has no composite pin. It drives four luma lines and one COLOR
    pin, and its fifteen colour codes select fifteen taps of one delay line,
    so colour is a phase, not a level: a pattern finer than the colour cycle
    (320 modes) makes colour of its own, which is how Tower Toppler draws
    its tower. A palette lookup cannot give that; this does. The encoder
    (composite_out) turns the codes into baseband composite in volts, with
    the measured luma averaging and chroma slew and the board's chroma
    resonator; the decoder (composite_decoder) splits luma from chroma with
    a half-subcarrier notch, demodulates against a local carrier and
    corrects against the burst it measures, the way a cheap set does.

    Both run at 4 samples a colour cycle (14.318 MHz, two a MARIA clock).
    The settings are MiSTer's: smear 1-3, AGC on or off, comb off or on, no
    setup (as NTSC-J), luma gain 2857, luma delay 2 samples. The artifact
    phase is where MARIA's pixels sit against the carrier, which turns the
    artifact colours (not the plain ones) by 90 degrees a step: 1 is the
    core's.

***************************************************************************/

#ifndef MAME_ATARI_A7800_COMPOSITE_H
#define MAME_ATARI_A7800_COMPOSITE_H

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>

class a7800_composite
{
public:
	// MARIA's line, in its clocks: sync, blanking, the burst, the picture
	static constexpr int LINE_CLOCKS = 454;
	static constexpr int SAMPLES = 2 * LINE_CLOCKS;  // 4 a colour cycle: 227 cycles a line
	static constexpr int HSYNC_END = 34;
	static constexpr int HBLANK_START = 440;
	static constexpr int HBLANK_END = 68;
	static constexpr int PICTURE_START = 93;         // the 320 clocks of the picture start here
	static constexpr int LATENCY = 14;               // samples from a code in to its luma out (an edge's half-way point)

	struct settings
	{
		int smear = 1;          // chroma trail, 1-3 (0: none)
		bool agc = true;        // track the level off the burst
		bool comb = false;      // average each line with the one above
		int phase_adj = 1;      // MARIA's pixels against the carrier, 0-3
		int temp = 0;           // the colour codes' delay line: 0 warm, 1 cool, 2 hot
	};

	struct input
	{
		uint8_t col, lum;       // colour code (0: no chroma) and luma code
		bool blank, hsync, hblank, vblank, vsync, pix_tick;
	};

	struct output
	{
		uint8_t r, g, b;
		bool hs, vs, hb, vb;
	};

	a7800_composite() : m_cache(new cache_entry[CACHE_SIZE]) { reset(); }

	void reset()
	{
		m_e = encoder();
		m_d = decoder();
		std::memset(m_linebuf, 0, sizeof(m_linebuf));
		for (int i = 0; i < CACHE_SIZE; i++)
			m_cache[i].valid = false;
	}
	void set(const settings &s) { m_s = s; }
	const settings &get() const { return m_s; }
	uint64_t lines_decoded() const { return m_decoded; }
	uint64_t lines_cached() const { return m_reused; }

	// one clock of both stages: MARIA's codes in, the decoder's RGB out
	output clock(const input &in)
	{
		const encoder::out e = m_e.step(in, m_s);
		return m_d.step(e, m_s, m_linebuf);
	}

	// One line: codes[c] is (colour << 4) | luma for each of MARIA's clocks
	// (blanking and sync are added here), rgb[] gets the decoder's output for
	// each sample, LATENCY samples after the code it is for.
	//
	// A line whose codes, settings and starting state (every register, and
	// the comb's line when it is on) are those of a line decoded before gives
	// what it gave, and leaves the state it left: that is exact, and most of a
	// picture's lines repeat, across the screen or from frame to frame.
	void line(const uint8_t *codes, bool vblank, bool vsync, uint32_t *rgb)
	{
		uint64_t key = 14695981039346656037ULL;
		auto hash = [&key] (const void *p, size_t n)
		{
			const uint8_t *b = static_cast<const uint8_t *>(p);
			for (size_t i = 0; i < n; i++)
				key = (key ^ b[i]) * 1099511628211ULL;
		};
		const int64_t flags[] = { vblank, vsync, m_s.smear, m_s.agc, m_s.comb, m_s.phase_adj, m_s.temp };
		hash(codes, LINE_CLOCKS);
		hash(flags, sizeof(flags));
		hash(&m_e, sizeof(m_e));
		hash(&m_d, sizeof(m_d));
		if (m_s.comb)
			hash(m_linebuf, sizeof(m_linebuf));
		cache_entry &ce = m_cache[key % CACHE_SIZE];
		if (ce.valid && ce.key == key
				&& !std::memcmp(ce.codes, codes, LINE_CLOCKS) && !std::memcmp(ce.flags, flags, sizeof(flags))
				&& !std::memcmp(&ce.e0, &m_e, sizeof(m_e)) && !std::memcmp(&ce.d0, &m_d, sizeof(m_d))
				&& (!m_s.comb || !std::memcmp(ce.lb0, m_linebuf, sizeof(m_linebuf))))
		{
			m_e = ce.e1;
			m_d = ce.d1;
			std::memcpy(m_linebuf, ce.lb1, sizeof(m_linebuf));
			std::memcpy(rgb, ce.rgb, sizeof(ce.rgb));
			m_reused++;
			return;
		}
		ce.valid = false;
		ce.key = key;
		std::memcpy(ce.codes, codes, LINE_CLOCKS);
		std::memcpy(ce.flags, flags, sizeof(flags));
		ce.e0 = m_e;
		ce.d0 = m_d;
		std::memcpy(ce.lb0, m_linebuf, sizeof(m_linebuf));

		decode(codes, vblank, vsync, rgb);
		m_decoded++;

		ce.e1 = m_e;
		ce.d1 = m_d;
		std::memcpy(ce.lb1, m_linebuf, sizeof(m_linebuf));
		std::memcpy(ce.rgb, rgb, sizeof(ce.rgb));
		ce.valid = true;
	}

	// the same, always clock by clock
	void decode(const uint8_t *codes, bool vblank, bool vsync, uint32_t *rgb)
	{
		input in;
		for (int s = 0; s < SAMPLES; s++)
		{
			const int c = s >> 1;
			in.col = codes[c] >> 4;
			in.lum = codes[c] & 0x0f;
			in.hsync = c < HSYNC_END;
			in.hblank = (c >= HBLANK_START) || (c < HBLANK_END);
			in.vblank = vblank;
			in.vsync = vsync;
			in.blank = in.hblank || vblank;
			in.pix_tick = !(s & 1);
			const output o = clock(in);
			rgb[s] = (uint32_t(o.r) << 16) | (uint32_t(o.g) << 8) | o.b;
		}
	}

private:
	// sign-extend the low n bits
	static int64_t sx(int64_t v, int n) { return int64_t(uint64_t(v) << (64 - n)) >> (64 - n); }
	// v[hi:lo], signed
	static int64_t bits(int64_t v, int hi, int lo) { return sx(v >> lo, hi - lo + 1); }

	// ---------------------------------------------------------------- composite_out

	struct encoder
	{
		// the burst, 40 IRE peak to peak, and the colour tables (temperature, colour code)
		static constexpr int64_t BURST_AMP = 299892;
		static constexpr int64_t RES_B = 3109, RES_A1 = 2249, RES_A2 = -10167;   // the chroma resonator, Q14
		static constexpr int LUMA_PEAK = 72;
		static constexpr int LUMA_POLE = 2;
		static constexpr int BURST_ON = 8, BURST_OFF = 44;      // MARIA's, samples after HSync falls
		static constexpr int CHROMA_SCALE = 256;

		static int64_t ld(int l)
		{
			static constexpr int32_t t[16] = {
					0, 76336, 234881, 364066, 481506, 593075, 698771, 798595,
					892548, 986500, 1074581, 1162661, 1250741, 1332950, 1415158, 1497367 };
			return t[l & 15];
		}
		static int64_t cs(int a)
		{
			static constexpr int32_t t[64] = {
					0, -246859, -133480, 23372, 195020, 305118, 332160, 278357,
					181065, 55346, -96085, -242472, -355201, -357886, -291383, -193908,
					0, -246859, -138802, 9787, 178773, 293292, 333673, 293864,
					209533, 95395, -45290, -189668, -320206, -367714, -328946, -249389,
					0, -246859, -129467, 38161, 212938, 316477, 326169, 257340,
					148035, 9565, -150468, -297168, -368547, -331017, -243682, -126855,
					0, -246859, -133480, 23372, 195020, 305118, 332160, 278357,
					181065, 55346, -96085, -242472, -355201, -357886, -291383, -193908 };
			return t[a & 63];
		}
		static int64_t cc(int a)
		{
			static constexpr int32_t t[64] = {
					0, 334654, 434776, 424674, 309252, 152053, -17466, -181253,
					-340097, -451450, -436775, -317650, -116341, 89767, 261507, 394020,
					0, 334654, 431555, 429254, 324568, 178783, 13652, -144969,
					-300489, -429602, -456519, -373598, -201114, 1872, 181176, 330729,
					0, 334654, 436445, 418126, 292213, 125079, -50498, -221290,
					-378630, -461572, -404615, -244498, -21959, 175325, 337185, 437380,
					0, 334654, 434776, 424674, 309252, 152053, -17466, -181253,
					-340097, -451450, -436775, -317650, -116341, 89767, 261507, 394020 };
			return t[a & 63];
		}

		struct out { int64_t comp; bool hs, vs, hb, vb; };

		// registers, all int64_t so that the state has no padding and compares with memcmp
		int64_t phase = 0, pixph = 0;
		int64_t hs_d = 0;
		int64_t hcnt = 0;
		int64_t cs_d = 0, cc_d = 0, lp = 0;
		int64_t cx1 = 0, cx2 = 0, cy1 = 0, cy2 = 0;
		int64_t lbuf0 = 0, lbuf1 = 0, lsum = 0;
		int64_t comp = 0;
		int64_t hs_out = 0, vs_out = 0, hb_out = 0, vb_out = 0;

		out step(const input &in, const settings &st)
		{
			const out o{ comp, hs_out != 0, vs_out != 0, hb_out != 0, vb_out != 0 };   // the registered outputs

			const int cphase = int(phase + st.phase_adj) & 3;
			const bool in_burst = (hcnt >= BURST_ON) && (hcnt < BURST_OFF);
			const int idx = ((st.temp & 3) << 4) | (in.col & 15);
			const int64_t cs_now = bits(cs(idx) * CHROMA_SCALE, 31, 8);
			const int64_t cc_now = bits(cc(idx) * CHROMA_SCALE, 31, 8);
			const int64_t chroma_s = bits(cs_now + cs_d, 24, 1);    // CHROMA_SLEW
			const int64_t chroma_c = bits(cc_now + cc_d, 24, 1);

			int64_t chroma, luma;
			if (in_burst)
			{
				luma = 0;
				chroma = (cphase == 1) ? -BURST_AMP : (cphase == 3) ? BURST_AMP : 0;
			}
			else if (in.blank)
			{
				luma = 0;
				chroma = 0;
			}
			else
			{
				luma = ld(in.lum);
				switch (cphase)
				{
				case 0:  chroma = chroma_s;  break;
				case 1:  chroma = chroma_c;  break;
				case 2:  chroma = -chroma_s; break;
				default: chroma = -chroma_c; break;
				}
				chroma = sx(chroma, 24);
			}

			// the board's series LC on the chroma
			const int64_t cdx = sx(chroma - cx2, 25);
			const int64_t cacc = sx(cdx * RES_B + cy1 * RES_A1 + cy2 * RES_A2, 41);
			const int64_t cres = bits(cacc, 40, 14);
			const int64_t chroma_ac = std::clamp<int64_t>(cres, -8388608, 8388607);

			// luma: a two-sample mean pulled toward its peak, then one pole
			const int64_t lsum_n = sx(lsum + luma - lbuf1, 26);
			const int64_t lmean = bits(lsum_n, 24, 1);
			const int64_t lmax = std::max(luma, lbuf0);
			const int64_t ldif = sx(lmax - lmean, 25);
			const int64_t lpk = sx(ldif * LUMA_PEAK, 33);
			const int64_t luma_pk = sx(lmean + bits(lpk, 31, 8), 24);
			const int64_t lerr = sx(luma_pk - lp, 25);
			const int64_t lp_next = sx(lp + (lerr >> LUMA_POLE), 25);

			cs_d = cs_now;
			cc_d = cc_now;
			comp = sx(lp + chroma_ac, 24);
			lp = sx(lp_next, 24);
			cx2 = cx1;
			cx1 = chroma;
			cy2 = cy1;
			cy1 = chroma_ac;
			lbuf1 = lbuf0;
			lbuf0 = luma;
			lsum = lsum_n;
			if (in.pix_tick)
			{
				phase = pixph;
				pixph = (pixph + 2) & 3;    // two samples a MARIA pixel
			}
			else
				phase = (phase + 1) & 3;

			hs_out = in.hsync;
			vs_out = in.vsync;
			hb_out = in.hblank;
			vb_out = in.vblank;

			if (hs_d && !in.hsync)
				hcnt = 0;
			else if (hcnt != 1023)
				hcnt++;
			hs_d = in.hsync;
			return o;
		}
	};

	// ------------------------------------------------------------ composite_decoder

	static constexpr int SPC = 4;

	struct decoder
	{
		static constexpr int BURST_START = 12, BURST_LEN = 36;   // MARIA's
		static constexpr int BURST_ACC = 16, BURST_SH = 4;
		static constexpr int CLAMP_ACC = 16, CLAMP_SH = 4;
		static constexpr uint32_t MIN_MAG2 = 16384;
		static constexpr uint32_t BURST_PP = 2343;
		static constexpr int BURST_SHIFT = 8;
		static constexpr int BURST_CONV = 128;
		static constexpr uint32_t DIV_NUM = 0x49f700a6;
		static constexpr uint32_t PHASE_INC = 16777216 / SPC;
		static constexpr int64_t BOX_RECIP = 65536 / SPC;
		static constexpr int SAT = 128, HUE = 0, LUMA_DELAY = 2;
		static constexpr int64_t SETUP = 0;
		static constexpr int64_t LUMA_GAIN = 2857;
		static constexpr int LAT = 12;

		static int sine(int p)
		{
			static constexpr int16_t q[65] = {
					0, 13, 25, 38, 50, 63, 75, 87, 100, 112, 124, 136, 148, 160, 172, 184,
					196, 207, 218, 230, 241, 252, 263, 273, 284, 294, 304, 314, 324, 334, 343, 352,
					361, 370, 379, 387, 395, 403, 410, 418, 425, 432, 438, 445, 451, 456, 462, 467,
					472, 477, 481, 485, 489, 492, 496, 499, 501, 503, 505, 507, 509, 510, 510, 511,
					511 };
			p &= 0xff;
			const int i = (p & 0x40) ? (64 - (p & 0x3f)) : (p & 0x3f);
			return (p & 0x80) ? -q[i] : q[i];
		}
		static int64_t sat16(int64_t v) { return std::clamp<int64_t>(v, -32768, 32767); }
		static int64_t clamp8(int64_t v) { return std::clamp<int64_t>(bits(v, 27, 8), 0, 255); }

		// registers, all int64_t (see the encoder's)
		int64_t hs_d = 0;
		int64_t hcnt = 0, hcnt_d = 0;
		int64_t black_acc = 0, black = 0, c16 = 0, prev = 0;
		int64_t nd0 = 0, nd1 = 0, yc = 0, cc = 0, yc_d = 0, y_lp = 0;
		int64_t phase = 0;
		int64_t i_dem = 0, q_dem = 0;
		int64_t ibuf[SPC] = { }, qbuf[SPC] = { };   // rings: [bpos] is the oldest
		int64_t bpos = 0;
		int64_t iacc = 0, qacc = 0, ibox = 0, qbox = 0;
		int64_t bmax = 0, bmin = 0;
		int64_t bpp = 0;
		int64_t ib_acc = 0, qb_acc = 0, ib = 0, qb = 0, ibh = 0, qbh = 0, cg = 0, sg = 0;
		int64_t div_den = 0, div_num = 0;
		int64_t div_cnt = 0;
		int64_t div_go = 0;
		int64_t inv = 0;
		int64_t colour_ok = 0;
		int64_t ag_den = 0, ag_num = 0;
		int64_t ag_cnt = 0;
		int64_t ag_ratio = 0, lgain = 0;
		int64_t i_cor = 0, q_cor = 0, i_sm = 0, q_sm = 0;
		int64_t ybuf[16] = { };   // a ring: ybuf[(ypos + k) & 15] is the sample k back
		int64_t ypos = 0;
		int64_t y_dly = 0, y8 = 0, i8 = 0, q8 = 0;
		int64_t r_out = 0, g_out = 0, b_out = 0;
		int64_t hs_p = 0, vs_p = 0, hb_p = 0, vb_p = 0;
		int64_t hs_out = 0, vs_out = 0, hb_out = 0, vb_out = 0;

		// Every register's next value comes from the current ones, then all change at once, as
		// on the clock edge.
		output step(const encoder::out &in, const settings &st, uint16_t *linebuf)
		{
			const output o{ uint8_t(r_out), uint8_t(g_out), uint8_t(b_out), hs_out != 0, vs_out != 0, hb_out != 0, vb_out != 0 };
			const int64_t comp = in.comp;

			// line timing
			const int burst_end = BURST_START + BURST_ACC;
			const int clamp_beg = BURST_START + BURST_LEN;
			const int clamp_end = clamp_beg + CLAMP_ACC;
			const bool in_burst = (hcnt >= BURST_START) && (hcnt < burst_end);
			const bool in_clamp = (hcnt >= clamp_beg) && (hcnt < clamp_end) && in.hb;
			const bool burst_fin = hcnt == burst_end;
			const bool clamp_fin = hcnt == clamp_end;

			// clamp and requantise
			const int64_t black_acc_n = (hcnt == 0) ? 0 : in_clamp ? sx(black_acc + comp, 29) : black_acc;
			const int64_t black_n = clamp_fin ? bits(black_acc, 27, CLAMP_SH) : black;
			const int64_t c16_n = sat16(sx(comp - black, 25) >> 8);

			// comb: a line back, read before this sample is written
			const int64_t prev_n = int16_t(linebuf[hcnt & 1023]);
			const int64_t cs = st.comb ? bits(c16 + prev, 16, 1) : c16;

			// notch
			const int64_t yc_n = bits(cs + nd1, 16, 1);
			const int64_t cc_n = bits(cs - nd1, 16, 1);
			const int64_t y_lp_n = bits(yc + yc_d, 16, 1);

			// carrier and mix
			const int ph = int(phase >> 16) & 0xff;
			const int64_t i_dem_n = bits(sx(cc * sine(ph), 27), 26, 9);
			const int64_t q_dem_n = bits(sx(cc * sine(ph + 64), 27), 26, 9);

			// chroma band limit
			const int64_t iacc_n = sx(iacc + i_dem - ibuf[bpos], 22);
			const int64_t qacc_n = sx(qacc + q_dem - qbuf[bpos], 22);
			const int64_t ibox_n = bits(iacc_n * BOX_RECIP, 33, 16);
			const int64_t qbox_n = bits(qacc_n * BOX_RECIP, 33, 16);

			// burst lock
			int64_t ib_acc_n = ib_acc, qb_acc_n = qb_acc, bmax_n = bmax, bmin_n = bmin;
			if (hcnt == 0)
			{
				ib_acc_n = qb_acc_n = 0;
				bmax_n = -32768;
				bmin_n = 32767;
			}
			else if (in_burst)
			{
				ib_acc_n = sx(ib_acc + ibox, 22);
				qb_acc_n = sx(qb_acc + qbox, 22);
				if (c16 > bmax) bmax_n = c16;
				if (c16 < bmin) bmin_n = c16;
			}
			int64_t ib_n = ib, qb_n = qb, bpp_n = bpp;
			if (burst_fin)
			{
				ib_n = sat16(bits(ib_acc, 21, BURST_SH));
				qb_n = sat16(bits(qb_acc, 21, BURST_SH));
				bpp_n = (bmax > bmin) ? (bmax - bmin) & 0xffff : 0;
			}

			// hue goes on the burst vector, once a line
			const int hue_ph = (HUE + BURST_CONV) & 0xff;
			const int64_t hue_c = sine(hue_ph + 64), hue_s = sine(hue_ph);
			const int64_t rot_i = sx(ib * hue_c - qb * hue_s, 27);
			const int64_t rot_q = sx(ib * hue_s + qb * hue_c, 27);
			const int64_t mag2 = int64_t(uint32_t(uint32_t(ibh * ibh) + uint32_t(qbh * qbh)));

			// the once-a-line divisions, inv = DIV_NUM / mag2 and the AGC's ratio, 32 steps each:
			// their results land on the last step
			int64_t ibh_n = ibh, qbh_n = qbh;
			int64_t div_cnt_n = div_cnt, ag_cnt_n = ag_cnt;
			int64_t div_num_n = div_num, div_den_n = div_den, ag_num_n = ag_num, ag_den_n = ag_den;
			int64_t inv_n = inv, ag_ratio_n = ag_ratio;
			int64_t colour_ok_n = colour_ok;
			if (div_go)
			{
				ibh_n = bits(rot_i, 24, 9);
				qbh_n = bits(rot_q, 24, 9);
				div_cnt_n = 32;
				div_num_n = DIV_NUM;
				div_den_n = mag2;
				ag_cnt_n = 32;
				ag_num_n = BURST_PP << 14;
				ag_den_n = bpp;
			}
			else if (div_cnt)
			{
				div_cnt_n = div_cnt - 1;
				if (div_cnt == 1)
				{
					colour_ok_n = div_den >= MIN_MAG2;
					const int64_t q = div_den ? (div_num / div_den) : 0xffffffff;
					inv_n = (q > 0xffff) ? 0xffff : q;
				}
			}
			if (ag_cnt)
			{
				ag_cnt_n = ag_cnt - 1;
				if (ag_cnt == 1)
				{
					// out of range: no usable burst, keep the fixed gain
					const int64_t q = ag_den ? (ag_num / ag_den) : 0xffffffff;
					ag_ratio_n = ((q >> 16) || (q & 0xffff) < 4096) ? 16384 : (q & 0xffff);
				}
			}
			const int64_t lg_agc = int64_t(uint32_t((LUMA_GAIN * ag_ratio) >> 14));
			const int64_t lgain_n = st.agc ? (lg_agc & 0x1ffff) : LUMA_GAIN;
			const int64_t cg_raw = bits(ibh * inv, BURST_SHIFT + 17, BURST_SHIFT);
			const int64_t sg_raw = bits(qbh * inv, BURST_SHIFT + 17, BURST_SHIFT);
			const int64_t cg_n = colour_ok ? bits(cg_raw * SAT, 24, 7) : 0;
			const int64_t sg_n = colour_ok ? bits(sg_raw * SAT, 24, 7) : 0;

			// burst correction and the chroma trail
			const int64_t i_cor_n = bits(ibox * cg + qbox * sg, 29, 12);
			const int64_t q_cor_n = bits(qbox * cg - ibox * sg, 29, 12);
			const int64_t i_err = sx(i_cor - i_sm, 18), q_err = sx(q_cor - q_sm, 18);
			const int64_t i_sm_n = st.smear ? sx(i_sm + (i_err >> st.smear), 18) : i_cor;
			const int64_t q_sm_n = st.smear ? sx(q_sm + (q_err >> st.smear), 18) : q_cor;

			// luma alignment
			const int64_t y_dly_n = ybuf[(ypos + LUMA_DELAY) & 15];

			// to RGB
			const int64_t y8_n = bits(sx(y_dly - SETUP, 17) * lgain, 33, 16);
			const int64_t i8_n = bits(i_sm * LUMA_GAIN, 33, 16);
			const int64_t q8_n = bits(q_sm * LUMA_GAIN, 33, 16);
			const int64_t y_sh = y8 << 8;
			const int64_t r_n = clamp8(sx(y_sh + (i8 * 245 + q8 * 159), 28));
			const int64_t g_n = clamp8(sx(y_sh - (i8 * 70 + q8 * 166), 28));
			const int64_t b_n = clamp8(sx(y_sh - (i8 * 283 - q8 * 436), 28));

			// ---- the clock edge
			linebuf[hcnt_d & 1023] = uint16_t(c16);
			hcnt_d = hcnt;
			prev = prev_n;
			black_acc = black_acc_n;
			black = black_n;
			c16 = c16_n;
			ypos = (ypos - 1) & 15;
			ybuf[ypos] = y_lp;
			nd1 = nd0;
			nd0 = cs;
			yc_d = yc;
			yc = yc_n;
			cc = cc_n;
			y_lp = y_lp_n;
			phase = (phase + int64_t(PHASE_INC)) & 0xffffff;
			ibuf[bpos] = i_dem;
			qbuf[bpos] = q_dem;
			bpos = (bpos + 1) & (SPC - 1);
			i_dem = i_dem_n;
			q_dem = q_dem_n;
			iacc = iacc_n;
			qacc = qacc_n;
			ibox = ibox_n;
			qbox = qbox_n;
			ib_acc = ib_acc_n;
			qb_acc = qb_acc_n;
			bmax = bmax_n;
			bmin = bmin_n;
			ib = ib_n;
			qb = qb_n;
			bpp = bpp_n;
			ibh = ibh_n;
			qbh = qbh_n;
			div_cnt = div_cnt_n;
			div_num = div_num_n;
			div_den = div_den_n;
			ag_cnt = ag_cnt_n;
			ag_num = ag_num_n;
			ag_den = ag_den_n;
			inv = inv_n;
			ag_ratio = ag_ratio_n;
			colour_ok = colour_ok_n;
			div_go = burst_fin;
			lgain = lgain_n;
			cg = cg_n;
			sg = sg_n;
			i_cor = i_cor_n;
			q_cor = q_cor_n;
			i_sm = i_sm_n;
			q_sm = q_sm_n;
			y_dly = y_dly_n;
			y8 = y8_n;
			i8 = i8_n;
			q8 = q8_n;
			r_out = r_n;
			g_out = g_n;
			b_out = b_n;

			hs_out = (hs_p >> (LAT - 1)) & 1;
			vs_out = (vs_p >> (LAT - 1)) & 1;
			hb_out = (hb_p >> (LAT - 1)) & 1;
			vb_out = (vb_p >> (LAT - 1)) & 1;
			hs_p = ((hs_p << 1) | in.hs) & ((1 << LAT) - 1);
			vs_p = ((vs_p << 1) | in.vs) & ((1 << LAT) - 1);
			hb_p = ((hb_p << 1) | in.hb) & ((1 << LAT) - 1);
			vb_p = ((vb_p << 1) | in.vb) & ((1 << LAT) - 1);

			if (hs_d && !in.hs)
				hcnt = 0;
			else if (hcnt != 1023)
				hcnt++;
			hs_d = in.hs;
			return o;
		}

	};

	static_assert(sizeof(encoder) % sizeof(int64_t) == 0, "the encoder's state must have no padding");
	static_assert(sizeof(decoder) % sizeof(int64_t) == 0, "the decoder's state must have no padding");

	static constexpr int CACHE_SIZE = 512;
	struct cache_entry
	{
		bool valid = false;
		uint64_t key = 0;
		uint8_t codes[LINE_CLOCKS];
		int64_t flags[7];
		encoder e0, e1;
		decoder d0, d1;
		uint16_t lb0[1024], lb1[1024];
		uint32_t rgb[SAMPLES];
	};

	settings m_s;
	encoder m_e;
	decoder m_d;
	uint16_t m_linebuf[1024];
	std::unique_ptr<cache_entry []> m_cache;
	uint64_t m_decoded = 0, m_reused = 0;
};

#endif // MAME_ATARI_A7800_COMPOSITE_H
