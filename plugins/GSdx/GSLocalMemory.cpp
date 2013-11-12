/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 *	Special Notes: 
 *
 *	Based on Page.c from GSSoft
 *	Copyright (C) 2002-2004 GSsoft Team
 *
 */
 
#include "StdAfx.h"
#include "GSLocalMemory.h"

#define ASSERT_BLOCK(r, w, h) \
	ASSERT((r).Width() >= w && (r).Height() >= h && !((r).left&(w-1)) && !((r).top&(h-1)) && !((r).right&(w-1)) && !((r).bottom&(h-1))); \

#define FOREACH_BLOCK_START(w, h, bpp) \
	DWORD bp = TEX0.TBP0; \
	DWORD bw = TEX0.TBW; \
	int offset = dstpitch * h - (r.right - r.left) * bpp / 8; \
	for(int y = r.top; y < r.bottom; y += h, dst += offset) \
	{ ASSERT_BLOCK(r, w, h); \
		for(int x = r.left; x < r.right; x += w, dst += w * bpp / 8) \
		{ \

#define FOREACH_BLOCK_END }}

//

DWORD GSLocalMemory::pageOffset32[32][32][64];
DWORD GSLocalMemory::pageOffset32Z[32][32][64];
DWORD GSLocalMemory::pageOffset16[32][64][64];
DWORD GSLocalMemory::pageOffset16S[32][64][64];
DWORD GSLocalMemory::pageOffset16Z[32][64][64];
DWORD GSLocalMemory::pageOffset16SZ[32][64][64];
DWORD GSLocalMemory::pageOffset8[32][64][128];
DWORD GSLocalMemory::pageOffset4[32][128][128];

int GSLocalMemory::rowOffset32[2048];
int GSLocalMemory::rowOffset32Z[2048];
int GSLocalMemory::rowOffset16[2048];
int GSLocalMemory::rowOffset16S[2048];
int GSLocalMemory::rowOffset16Z[2048];
int GSLocalMemory::rowOffset16SZ[2048];
int GSLocalMemory::rowOffset8[2][2048];
int GSLocalMemory::rowOffset4[2][2048];

//

DWORD GSLocalMemory::m_xtbl[1024];
DWORD GSLocalMemory::m_ytbl[1024]; 

//

GSLocalMemory::psm_t GSLocalMemory::m_psm[64];

//

GSLocalMemory::GSLocalMemory()
	: m_clut(this)
{
	m_vm8 = (BYTE*)VirtualAlloc(NULL, m_vmsize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	memset(m_vm8, 0, m_vmsize);

	for(int bp = 0; bp < 32; bp++)
	{
		for(int y = 0; y < 32; y++) for(int x = 0; x < 64; x++)
		{
			pageOffset32[bp][y][x] = PixelAddressOrg32(x, y, bp, 0);
			pageOffset32Z[bp][y][x] = PixelAddressOrg32Z(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 64; x++) 
		{
			pageOffset16[bp][y][x] = PixelAddressOrg16(x, y, bp, 0);
			pageOffset16S[bp][y][x] = PixelAddressOrg16S(x, y, bp, 0);
			pageOffset16Z[bp][y][x] = PixelAddressOrg16Z(x, y, bp, 0);
			pageOffset16SZ[bp][y][x] = PixelAddressOrg16SZ(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset8[bp][y][x] = PixelAddressOrg8(x, y, bp, 0);
		}

		for(int y = 0; y < 128; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset4[bp][y][x] = PixelAddressOrg4(x, y, bp, 0);
		}
	}

	for(int x = 0; x < countof(rowOffset32); x++)
	{
		rowOffset32[x] = (int)PixelAddress32(x, 0, 0, 32) - (int)PixelAddress32(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset32Z); x++)
	{
		rowOffset32Z[x] = (int)PixelAddress32Z(x, 0, 0, 32) - (int)PixelAddress32Z(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16); x++)
	{
		rowOffset16[x] = (int)PixelAddress16(x, 0, 0, 32) - (int)PixelAddress16(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16S); x++)
	{
		rowOffset16S[x] = (int)PixelAddress16S(x, 0, 0, 32) - (int)PixelAddress16S(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16Z); x++)
	{
		rowOffset16Z[x] = (int)PixelAddress16Z(x, 0, 0, 32) - (int)PixelAddress16Z(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16SZ); x++)
	{
		rowOffset16SZ[x] = (int)PixelAddress16SZ(x, 0, 0, 32) - (int)PixelAddress16SZ(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset8[0]); x++)
	{
		rowOffset8[0][x] = (int)PixelAddress8(x, 0, 0, 32) - (int)PixelAddress8(0, 0, 0, 32),
		rowOffset8[1][x] = (int)PixelAddress8(x, 2, 0, 32) - (int)PixelAddress8(0, 2, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset4[0]); x++)
	{
		rowOffset4[0][x] = (int)PixelAddress4(x, 0, 0, 32) - (int)PixelAddress4(0, 0, 0, 32),
		rowOffset4[1][x] = (int)PixelAddress4(x, 2, 0, 32) - (int)PixelAddress4(0, 2, 0, 32);
	}

	for(int i = 0; i < countof(m_psm); i++)
	{
		m_psm[i].pa = &GSLocalMemory::PixelAddress32;
		m_psm[i].ba = &GSLocalMemory::BlockAddress32;
		m_psm[i].pga = &GSLocalMemory::PageAddress32;
		m_psm[i].pgn = &GSLocalMemory::PageNumber32;
		m_psm[i].rp = &GSLocalMemory::ReadPixel32;
		m_psm[i].rpa = &GSLocalMemory::ReadPixel32;
		m_psm[i].wp = &GSLocalMemory::WritePixel32;
		m_psm[i].wpa = &GSLocalMemory::WritePixel32;
		m_psm[i].rt = &GSLocalMemory::ReadTexel32;
		m_psm[i].rtNP = &GSLocalMemory::ReadTexel32;
		m_psm[i].rta = &GSLocalMemory::ReadTexel32;
		m_psm[i].wfa = &GSLocalMemory::WritePixel32;
		m_psm[i].wi = &GSLocalMemory::WriteImage<PSM_PSMCT32, 8, 8, 32>;
		m_psm[i].ri = &GSLocalMemory::ReadImageX; // TODO
		m_psm[i].rtx = &GSLocalMemory::ReadTexture32;
		m_psm[i].rtxNP = &GSLocalMemory::ReadTexture32;
		m_psm[i].rtxP = &GSLocalMemory::ReadTexture32;
		m_psm[i].bpp = m_psm[i].trbpp = 32;
		m_psm[i].pal = 0;
		m_psm[i].bs = CSize(8, 8);
		m_psm[i].pgs = CSize(64, 32);
		for(int j = 0; j < 8; j++) m_psm[i].rowOffset[j] = rowOffset32;
	}

	m_psm[PSM_PSMCT16].pa = &GSLocalMemory::PixelAddress16;
	m_psm[PSM_PSMCT16S].pa = &GSLocalMemory::PixelAddress16S;
	m_psm[PSM_PSMT8].pa = &GSLocalMemory::PixelAddress8;
	m_psm[PSM_PSMT4].pa = &GSLocalMemory::PixelAddress4;
	m_psm[PSM_PSMZ32].pa = &GSLocalMemory::PixelAddress32Z;
	m_psm[PSM_PSMZ24].pa = &GSLocalMemory::PixelAddress32Z;
	m_psm[PSM_PSMZ16].pa = &GSLocalMemory::PixelAddress16Z;
	m_psm[PSM_PSMZ16S].pa = &GSLocalMemory::PixelAddress16SZ;

	m_psm[PSM_PSMCT16].ba = &GSLocalMemory::BlockAddress16;
	m_psm[PSM_PSMCT16S].ba = &GSLocalMemory::BlockAddress16S;
	m_psm[PSM_PSMT8].ba = &GSLocalMemory::BlockAddress8;
	m_psm[PSM_PSMT4].ba = &GSLocalMemory::BlockAddress4;
	m_psm[PSM_PSMZ32].ba = &GSLocalMemory::BlockAddress32Z;
	m_psm[PSM_PSMZ24].ba = &GSLocalMemory::BlockAddress32Z;
	m_psm[PSM_PSMZ16].ba = &GSLocalMemory::BlockAddress16Z;
	m_psm[PSM_PSMZ16S].ba = &GSLocalMemory::BlockAddress16SZ;

	m_psm[PSM_PSMCT16].pga = &GSLocalMemory::PageAddress16;
	m_psm[PSM_PSMCT16S].pga = &GSLocalMemory::PageAddress16;
	m_psm[PSM_PSMZ16].pga = &GSLocalMemory::PageAddress16;
	m_psm[PSM_PSMZ16S].pga = &GSLocalMemory::PageAddress16;
	m_psm[PSM_PSMT8].pga = &GSLocalMemory::PageAddress8;
	m_psm[PSM_PSMT4].pga = &GSLocalMemory::PageAddress4;

	m_psm[PSM_PSMCT16].pgn = &GSLocalMemory::PageNumber16;
	m_psm[PSM_PSMCT16S].pgn = &GSLocalMemory::PageNumber16;
	m_psm[PSM_PSMZ16].pgn = &GSLocalMemory::PageNumber16;
	m_psm[PSM_PSMZ16S].pgn = &GSLocalMemory::PageNumber16;
	m_psm[PSM_PSMT8].pgn = &GSLocalMemory::PageNumber8;
	m_psm[PSM_PSMT4].pgn = &GSLocalMemory::PageNumber4;

	m_psm[PSM_PSMCT24].rp = &GSLocalMemory::ReadPixel24;
	m_psm[PSM_PSMCT16].rp = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMCT16S].rp = &GSLocalMemory::ReadPixel16S;
	m_psm[PSM_PSMT8].rp = &GSLocalMemory::ReadPixel8;
	m_psm[PSM_PSMT4].rp = &GSLocalMemory::ReadPixel4;
	m_psm[PSM_PSMT8H].rp = &GSLocalMemory::ReadPixel8H;
	m_psm[PSM_PSMT4HL].rp = &GSLocalMemory::ReadPixel4HL;
	m_psm[PSM_PSMT4HH].rp = &GSLocalMemory::ReadPixel4HH;
	m_psm[PSM_PSMZ32].rp = &GSLocalMemory::ReadPixel32Z;
	m_psm[PSM_PSMZ24].rp = &GSLocalMemory::ReadPixel24Z;
	m_psm[PSM_PSMZ16].rp = &GSLocalMemory::ReadPixel16Z;
	m_psm[PSM_PSMZ16S].rp = &GSLocalMemory::ReadPixel16SZ;

	m_psm[PSM_PSMCT24].rpa = &GSLocalMemory::ReadPixel24;
	m_psm[PSM_PSMCT16].rpa = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMCT16S].rpa = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMT8].rpa = &GSLocalMemory::ReadPixel8;
	m_psm[PSM_PSMT4].rpa = &GSLocalMemory::ReadPixel4;
	m_psm[PSM_PSMT8H].rpa = &GSLocalMemory::ReadPixel8H;
	m_psm[PSM_PSMT4HL].rpa = &GSLocalMemory::ReadPixel4HL;
	m_psm[PSM_PSMT4HH].rpa = &GSLocalMemory::ReadPixel4HH;
	m_psm[PSM_PSMZ32].rpa = &GSLocalMemory::ReadPixel32;
	m_psm[PSM_PSMZ24].rpa = &GSLocalMemory::ReadPixel24;
	m_psm[PSM_PSMZ16].rpa = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMZ16S].rpa = &GSLocalMemory::ReadPixel16;

	m_psm[PSM_PSMCT32].wp = &GSLocalMemory::WritePixel32;
	m_psm[PSM_PSMCT24].wp = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMCT16].wp = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMCT16S].wp = &GSLocalMemory::WritePixel16S;
	m_psm[PSM_PSMT8].wp = &GSLocalMemory::WritePixel8;
	m_psm[PSM_PSMT4].wp = &GSLocalMemory::WritePixel4;
	m_psm[PSM_PSMT8H].wp = &GSLocalMemory::WritePixel8H;
	m_psm[PSM_PSMT4HL].wp = &GSLocalMemory::WritePixel4HL;
	m_psm[PSM_PSMT4HH].wp = &GSLocalMemory::WritePixel4HH;
	m_psm[PSM_PSMZ32].wp = &GSLocalMemory::WritePixel32Z;
	m_psm[PSM_PSMZ24].wp = &GSLocalMemory::WritePixel24Z;
	m_psm[PSM_PSMZ16].wp = &GSLocalMemory::WritePixel16Z;
	m_psm[PSM_PSMZ16S].wp = &GSLocalMemory::WritePixel16SZ;

	m_psm[PSM_PSMCT32].wpa = &GSLocalMemory::WritePixel32;
	m_psm[PSM_PSMCT24].wpa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMCT16].wpa = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMCT16S].wpa = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMT8].wpa = &GSLocalMemory::WritePixel8;
	m_psm[PSM_PSMT4].wpa = &GSLocalMemory::WritePixel4;
	m_psm[PSM_PSMT8H].wpa = &GSLocalMemory::WritePixel8H;
	m_psm[PSM_PSMT4HL].wpa = &GSLocalMemory::WritePixel4HL;
	m_psm[PSM_PSMT4HH].wpa = &GSLocalMemory::WritePixel4HH;
	m_psm[PSM_PSMZ32].wpa = &GSLocalMemory::WritePixel32;
	m_psm[PSM_PSMZ24].wpa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMZ16].wpa = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMZ16S].wpa = &GSLocalMemory::WritePixel16;

	m_psm[PSM_PSMCT24].rt = &GSLocalMemory::ReadTexel24;
	m_psm[PSM_PSMCT16].rt = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMCT16S].rt = &GSLocalMemory::ReadTexel16S;
	m_psm[PSM_PSMT8].rt = &GSLocalMemory::ReadTexel8;
	m_psm[PSM_PSMT4].rt = &GSLocalMemory::ReadTexel4;
	m_psm[PSM_PSMT8H].rt = &GSLocalMemory::ReadTexel8H;
	m_psm[PSM_PSMT4HL].rt = &GSLocalMemory::ReadTexel4HL;
	m_psm[PSM_PSMT4HH].rt = &GSLocalMemory::ReadTexel4HH;
	m_psm[PSM_PSMZ32].rt = &GSLocalMemory::ReadTexel32Z;
	m_psm[PSM_PSMZ24].rt = &GSLocalMemory::ReadTexel24Z;
	m_psm[PSM_PSMZ16].rt = &GSLocalMemory::ReadTexel16Z;
	m_psm[PSM_PSMZ16S].rt = &GSLocalMemory::ReadTexel16SZ;

	m_psm[PSM_PSMCT24].rta = &GSLocalMemory::ReadTexel24;
	m_psm[PSM_PSMCT16].rta = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMCT16S].rta = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMT8].rta = &GSLocalMemory::ReadTexel8;
	m_psm[PSM_PSMT4].rta = &GSLocalMemory::ReadTexel4;
	m_psm[PSM_PSMT8H].rta = &GSLocalMemory::ReadTexel8H;
	m_psm[PSM_PSMT4HL].rta = &GSLocalMemory::ReadTexel4HL;
	m_psm[PSM_PSMT4HH].rta = &GSLocalMemory::ReadTexel4HH;
	m_psm[PSM_PSMZ24].rta = &GSLocalMemory::ReadTexel24;
	m_psm[PSM_PSMZ16].rta = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMZ16S].rta = &GSLocalMemory::ReadTexel16;

	m_psm[PSM_PSMCT24].wfa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMCT16].wfa = &GSLocalMemory::WriteFrame16;
	m_psm[PSM_PSMCT16S].wfa = &GSLocalMemory::WriteFrame16;
	m_psm[PSM_PSMZ24].wfa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMZ16].wfa = &GSLocalMemory::WriteFrame16;
	m_psm[PSM_PSMZ16S].wfa = &GSLocalMemory::WriteFrame16;

	m_psm[PSM_PSMCT16].rtNP = &GSLocalMemory::ReadTexel16NP;
	m_psm[PSM_PSMCT16S].rtNP = &GSLocalMemory::ReadTexel16SNP;
	m_psm[PSM_PSMT8].rtNP = &GSLocalMemory::ReadTexel8;
	m_psm[PSM_PSMT4].rtNP = &GSLocalMemory::ReadTexel4;
	m_psm[PSM_PSMT8H].rtNP = &GSLocalMemory::ReadTexel8H;
	m_psm[PSM_PSMT4HL].rtNP = &GSLocalMemory::ReadTexel4HL;
	m_psm[PSM_PSMT4HH].rtNP = &GSLocalMemory::ReadTexel4HH;
	m_psm[PSM_PSMZ32].rtNP = &GSLocalMemory::ReadTexel32Z;
	m_psm[PSM_PSMZ24].rtNP = &GSLocalMemory::ReadTexel24Z;
	m_psm[PSM_PSMZ16].rtNP = &GSLocalMemory::ReadTexel16ZNP;
	m_psm[PSM_PSMZ16S].rtNP = &GSLocalMemory::ReadTexel16SZNP;

	m_psm[PSM_PSMCT24].wi = &GSLocalMemory::WriteImage24; // TODO
	m_psm[PSM_PSMCT16].wi = &GSLocalMemory::WriteImage<PSM_PSMCT16, 16, 8, 16>;
	m_psm[PSM_PSMCT16S].wi = &GSLocalMemory::WriteImage<PSM_PSMCT16S, 16, 8, 16>;
	m_psm[PSM_PSMT8].wi = &GSLocalMemory::WriteImage<PSM_PSMT8, 16, 16, 8>;
	m_psm[PSM_PSMT4].wi = &GSLocalMemory::WriteImage<PSM_PSMT4, 32, 16, 4>;
	m_psm[PSM_PSMT8H].wi = &GSLocalMemory::WriteImage8H; // TODO
	m_psm[PSM_PSMT4HL].wi = &GSLocalMemory::WriteImage4HL; // TODO
	m_psm[PSM_PSMT4HH].wi = &GSLocalMemory::WriteImage4HH; // TODO
	m_psm[PSM_PSMZ32].wi = &GSLocalMemory::WriteImage<PSM_PSMZ32, 8, 8, 32>;
	m_psm[PSM_PSMZ24].wi = &GSLocalMemory::WriteImage24Z; // TODO
	m_psm[PSM_PSMZ16].wi = &GSLocalMemory::WriteImage<PSM_PSMZ16, 16, 8, 16>;
	m_psm[PSM_PSMZ16S].wi = &GSLocalMemory::WriteImage<PSM_PSMZ16S, 16, 8, 16>;

	m_psm[PSM_PSMCT24].rtx = &GSLocalMemory::ReadTexture24;
	m_psm[PSM_PSMCT16].rtx = &GSLocalMemory::ReadTexture16;
	m_psm[PSM_PSMCT16S].rtx = &GSLocalMemory::ReadTexture16S;
	m_psm[PSM_PSMT8].rtx = &GSLocalMemory::ReadTexture8;
	m_psm[PSM_PSMT4].rtx = &GSLocalMemory::ReadTexture4;
	m_psm[PSM_PSMT8H].rtx = &GSLocalMemory::ReadTexture8H;
	m_psm[PSM_PSMT4HL].rtx = &GSLocalMemory::ReadTexture4HL;
	m_psm[PSM_PSMT4HH].rtx = &GSLocalMemory::ReadTexture4HH;
	m_psm[PSM_PSMZ32].rtx = &GSLocalMemory::ReadTexture32Z;
	m_psm[PSM_PSMZ24].rtx = &GSLocalMemory::ReadTexture24Z;
	m_psm[PSM_PSMZ16].rtx = &GSLocalMemory::ReadTexture16Z;
	m_psm[PSM_PSMZ16S].rtx = &GSLocalMemory::ReadTexture16SZ;

	m_psm[PSM_PSMCT16].rtxNP = &GSLocalMemory::ReadTexture16NP;
	m_psm[PSM_PSMCT16S].rtxNP = &GSLocalMemory::ReadTexture16SNP;
	m_psm[PSM_PSMT8].rtxNP = &GSLocalMemory::ReadTexture8NP;
	m_psm[PSM_PSMT4].rtxNP = &GSLocalMemory::ReadTexture4NP;
	m_psm[PSM_PSMT8H].rtxNP = &GSLocalMemory::ReadTexture8HNP;
	m_psm[PSM_PSMT4HL].rtxNP = &GSLocalMemory::ReadTexture4HLNP;
	m_psm[PSM_PSMT4HH].rtxNP = &GSLocalMemory::ReadTexture4HHNP;
	m_psm[PSM_PSMZ32].rtxNP = &GSLocalMemory::ReadTexture32Z;
	m_psm[PSM_PSMZ24].rtxNP = &GSLocalMemory::ReadTexture24Z;
	m_psm[PSM_PSMZ16].rtxNP = &GSLocalMemory::ReadTexture16ZNP;
	m_psm[PSM_PSMZ16S].rtxNP = &GSLocalMemory::ReadTexture16SZNP;

	m_psm[PSM_PSMT8].rtxP = &GSLocalMemory::ReadTexture8P;
	m_psm[PSM_PSMT4].rtxP = &GSLocalMemory::ReadTexture4P;
	m_psm[PSM_PSMT8H].rtxP = &GSLocalMemory::ReadTexture8HP;
	m_psm[PSM_PSMT4HL].rtxP = &GSLocalMemory::ReadTexture4HLP;
	m_psm[PSM_PSMT4HH].rtxP = &GSLocalMemory::ReadTexture4HHP;

	m_psm[PSM_PSMT8].pal = m_psm[PSM_PSMT8H].pal = 256;
	m_psm[PSM_PSMT4].pal = m_psm[PSM_PSMT4HL].pal = m_psm[PSM_PSMT4HH].pal = 16;

	m_psm[PSM_PSMCT16].bpp = m_psm[PSM_PSMCT16S].bpp = 16;
	m_psm[PSM_PSMT8].bpp = 8;
	m_psm[PSM_PSMT4].bpp = 4;
	m_psm[PSM_PSMZ16].bpp = m_psm[PSM_PSMZ16S].bpp = 16;

	m_psm[PSM_PSMCT24].trbpp = 24;
	m_psm[PSM_PSMCT16].trbpp = m_psm[PSM_PSMCT16S].trbpp = 16;
	m_psm[PSM_PSMT8].trbpp = m_psm[PSM_PSMT8H].trbpp = 8;
	m_psm[PSM_PSMT4].trbpp = m_psm[PSM_PSMT4HL].trbpp = m_psm[PSM_PSMT4HH].trbpp = 4;
	m_psm[PSM_PSMZ24].trbpp = 24;
	m_psm[PSM_PSMZ16].trbpp = m_psm[PSM_PSMZ16S].trbpp = 16;

	m_psm[PSM_PSMCT16].bs = m_psm[PSM_PSMCT16S].bs = CSize(16, 8);
	m_psm[PSM_PSMT8].bs = CSize(16, 16);
	m_psm[PSM_PSMT4].bs = CSize(32, 16);
	m_psm[PSM_PSMZ16].bs = m_psm[PSM_PSMZ16S].bs = CSize(16, 8);

	m_psm[PSM_PSMCT16].pgs = m_psm[PSM_PSMCT16S].pgs = CSize(64, 64);
	m_psm[PSM_PSMT8].pgs = CSize(128, 64);
	m_psm[PSM_PSMT4].pgs = CSize(128, 128);
	m_psm[PSM_PSMZ16].pgs = m_psm[PSM_PSMZ16S].pgs = CSize(64, 64);

	for(int i = 0; i < 8; i++) m_psm[PSM_PSMCT16].rowOffset[i] = rowOffset16;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMCT16S].rowOffset[i] = rowOffset16S;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMT8].rowOffset[i] = rowOffset8[((i + 2) >> 2) & 1];
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMT4].rowOffset[i] = rowOffset4[((i + 2) >> 2) & 1];
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ32].rowOffset[i] = rowOffset32Z;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ24].rowOffset[i] = rowOffset32Z;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ16].rowOffset[i] = rowOffset16Z;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ16S].rowOffset[i] = rowOffset16SZ;
}

GSLocalMemory::~GSLocalMemory()
{
	VirtualFree(m_vm8, 0, MEM_RELEASE);

	POSITION pos = m_omap.GetHeadPosition();

	while(pos)
	{
		Offset* o = m_omap.GetNextValue(pos);

		for(int i = 0; i < countof(o->col); i++)
		{
			_aligned_free(o->col);
		}

		_aligned_free(o);
	}

	m_omap.RemoveAll();

	pos = m_o4map.GetHeadPosition();

	while(pos)
	{
		_aligned_free(m_o4map.GetNextValue(pos));
	}

	m_o4map.RemoveAll();
}

GSLocalMemory::Offset* GSLocalMemory::GetOffset(DWORD bp, DWORD bw, DWORD psm)
{
	if(bw == 0) {ASSERT(0); return NULL;}

	ASSERT(m_psm[psm].bpp > 8); // only for 16/24/32/8h/4hh/4hl formats where all columns are the same

	DWORD hash = bp | (bw << 14) | (psm << 20);

	if(CRBMap<DWORD, Offset*>::CPair* pair = m_omap.Lookup(hash))
	{
		return pair->m_value;
	}

	Offset* o = (Offset*)_aligned_malloc(sizeof(Offset), 16);

	o->hash = hash;

	pixelAddress pa = m_psm[psm].pa;

	for(int i = 0; i < 2048; i++)
	{
		o->row[i] = GSVector4i((int)pa(0, i, bp, bw));
	}

	int* p = (int*)_aligned_malloc(sizeof(int) * (2048 + 3) * 4, 16);

	for(int i = 0; i < 4; i++)
	{
		o->col[i] = &p[2048 * i + ((4 - (i & 3)) & 3)];

		memcpy(o->col[i], m_psm[psm].rowOffset[0], sizeof(int) * 2048);
	}

	m_omap.SetAt(hash, o);

	return o;
}

GSLocalMemory::Offset4* GSLocalMemory::GetOffset4(const GIFRegFRAME& FRAME, const GIFRegZBUF& ZBUF)
{
	DWORD fbp = FRAME.Block();
	DWORD zbp = ZBUF.Block();
	DWORD fpsm = FRAME.PSM;
	DWORD zpsm = ZBUF.PSM;
	DWORD bw = FRAME.FBW;

	ASSERT(m_psm[fpsm].trbpp > 8 || m_psm[zpsm].trbpp > 8);

	// "(psm & 0x0f) ^ ((psm & 0xf0) >> 2)" creates 4 bit unique identifiers for render target formats (only)

	DWORD fpsm_hash = (fpsm & 0x0f) ^ ((fpsm & 0x30) >> 2);
	DWORD zpsm_hash = (zpsm & 0x0f) ^ ((zpsm & 0x30) >> 2);

	DWORD hash = (FRAME.FBP << 0) | (ZBUF.ZBP << 9) | (bw << 18) | (fpsm_hash << 24) | (zpsm_hash << 28);

	if(CRBMap<DWORD, Offset4*>::CPair* pair = m_o4map.Lookup(hash))
	{
		return pair->m_value;
	}

	Offset4* o = (Offset4*)_aligned_malloc(sizeof(Offset4), 16);

	o->hash = hash;

	pixelAddress fpa = m_psm[fpsm].pa;
	pixelAddress zpa = m_psm[zpsm].pa;

	int fs = m_psm[fpsm].bpp >> 5;
	int zs = m_psm[zpsm].bpp >> 5;

	for(int i = 0; i < 2048; i++)
	{
		o->row[i].x = (int)fpa(0, i, fbp, bw) << fs;
		o->row[i].y = (int)zpa(0, i, zbp, bw) << zs;
	}

	for(int i = 0; i < 512; i++)
	{
		o->col[i].x = m_psm[fpsm].rowOffset[0][i * 4] << fs;
		o->col[i].y = m_psm[zpsm].rowOffset[0][i * 4] << zs;
	}

	m_o4map.SetAt(hash, o);

	return o;
}

bool GSLocalMemory::FillRect(const GSVector4i& r, DWORD c, DWORD psm, DWORD bp, DWORD bw)
{
	const psm_t& tbl = m_psm[psm];

	writePixel wp = tbl.wp;
	pixelAddress ba = tbl.ba;

	int w = tbl.bs.cx;
	int h = tbl.bs.cy;
	int bpp = tbl.bpp;

	int shift = 0;

	switch(bpp)
	{
	case 32: shift = 0; break;
	case 16: shift = 1; c = (c & 0xffff) * 0x00010001; break;
	case 8: shift = 2; c = (c & 0xff) * 0x01010101; break;
	case 4: shift = 3; c = (c & 0xf) * 0x11111111; break;
	}

	CRect clip;
	
	clip.left = (r.x + (w - 1)) & ~(w - 1);
	clip.top = (r.y + (h - 1)) & ~(h - 1);
	clip.right = r.z & ~(w - 1);
	clip.bottom = r.w & ~(h - 1);

	for(int y = r.y; y < clip.top; y++)
	{
		for(int x = r.x; x < r.z; x++)
		{
			(this->*wp)(x, y, c, bp, bw);
		}
	}

	for(int y = clip.bottom; y < r.w; y++)
	{
		for(int x = r.x; x < r.z; x++)
		{
			(this->*wp)(x, y, c, bp, bw);
		}
	}

	if(r.x < clip.left || clip.right < r.z)
	{
		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int ys = y, ye = y + h; ys < ye; ys++)
			{
				for(int x = r.x; x < clip.left; x++)
				{
					(this->*wp)(x, ys, c, bp, bw);
				}

				for(int x = clip.right; x < r.z; x++)
				{
					(this->*wp)(x, ys, c, bp, bw);
				}
			}
		}
	}

	if(psm == PSM_PSMCT24 || psm == PSM_PSMZ24)
	{
		#if _M_SSE >= 0x200

		GSVector4i c128(c);
		GSVector4i mask(0x00ffffff);

		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				GSVector4i* p = (GSVector4i*)&m_vm8[ba(x, y, bp, bw) << 2 >> shift];

				for(int i = 0; i < 16; i += 4)
				{
					p[i + 0] = p[i + 0].blend8(c128, mask);
					p[i + 1] = p[i + 1].blend8(c128, mask);
					p[i + 2] = p[i + 2].blend8(c128, mask);
					p[i + 3] = p[i + 3].blend8(c128, mask);
				}
			}
		}

		#else

		c &= 0x00ffffff;

		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				DWORD* p = &m_vm32[ba(x, y, bp, bw)];

				for(int i = 0; i < 64; i += 4)
				{
					p[i + 0] = (p[i + 0] & 0xff000000) | c;
					p[i + 1] = (p[i + 1] & 0xff000000) | c;
					p[i + 2] = (p[i + 2] & 0xff000000) | c;
					p[i + 3] = (p[i + 3] & 0xff000000) | c;
				}
			}
		}

		#endif
	}
	else
	{
		#if _M_SSE >= 0x200

		GSVector4i c128(c);

		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				GSVector4i* p = (GSVector4i*)&m_vm8[ba(x, y, bp, bw) << 2 >> shift];

				for(int i = 0; i < 16; i += 4)
				{
					p[i + 0] = c128;
					p[i + 1] = c128;
					p[i + 2] = c128;
					p[i + 3] = c128;
				}
			}
		}

		#else

		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				DWORD* p = (DWORD*)&m_vm8[ba(x, y, bp, bw) << 2 >> shift];

				for(int i = 0; i < 64; i += 4)
				{
					p[i + 0] = c;
					p[i + 1] = c;
					p[i + 2] = c;
					p[i + 3] = c;
				}
			}
		}

		#endif
	}

	return true;
}

////////////////////

template<int psm, int bsx, int bsy, bool aligned>
void GSLocalMemory::WriteImageColumn(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	const int csy = bsy / 4;

	for(int offset = srcpitch * csy; h >= csy; h -= csy, y += csy, src += offset)
	{
		for(int x = l; x < r; x += bsx)
		{
			switch(psm)
			{
			case PSM_PSMCT32: WriteColumn32<aligned, 0xffffffff>(y, (BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], &src[x * 4], srcpitch); break;
			case PSM_PSMCT16: WriteColumn16<aligned>(y, (BYTE*)&m_vm16[BlockAddress16(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			case PSM_PSMCT16S: WriteColumn16<aligned>(y, (BYTE*)&m_vm16[BlockAddress16S(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			case PSM_PSMT8: WriteColumn8<aligned>(y, (BYTE*)&m_vm8[BlockAddress8(x, y, bp, bw)], &src[x], srcpitch); break;
			case PSM_PSMT4: WriteColumn4<aligned>(y, (BYTE*)&m_vm8[BlockAddress4(x, y, bp, bw) >> 1], &src[x >> 1], srcpitch); break;
			case PSM_PSMZ32: WriteColumn32<aligned, 0xffffffff>(y, (BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)], &src[x * 4], srcpitch); break;
			case PSM_PSMZ16: WriteColumn16<aligned>(y, (BYTE*)&m_vm16[BlockAddress16Z(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			case PSM_PSMZ16S: WriteColumn16<aligned>(y, (BYTE*)&m_vm16[BlockAddress16SZ(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			// TODO
			default: __assume(0);
			}
		}
	}
}

template<int psm, int bsx, int bsy, bool aligned>
void GSLocalMemory::WriteImageBlock(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	for(int offset = srcpitch * bsy; h >= bsy; h -= bsy, y += bsy, src += offset)
	{
		for(int x = l; x < r; x += bsx)
		{
			switch(psm)
			{
			case PSM_PSMCT32: WriteBlock32<aligned, 0xffffffff>((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], &src[x * 4], srcpitch); break;
			case PSM_PSMCT16: WriteBlock16<aligned>((BYTE*)&m_vm16[BlockAddress16(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			case PSM_PSMCT16S: WriteBlock16<aligned>((BYTE*)&m_vm16[BlockAddress16S(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			case PSM_PSMT8: WriteBlock8<aligned>((BYTE*)&m_vm8[BlockAddress8(x, y, bp, bw)], &src[x], srcpitch); break;
			case PSM_PSMT4: WriteBlock4<aligned>((BYTE*)&m_vm8[BlockAddress4(x, y, bp, bw) >> 1], &src[x >> 1], srcpitch); break;
			case PSM_PSMZ32: WriteBlock32<aligned, 0xffffffff>((BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)], &src[x * 4], srcpitch); break;
			case PSM_PSMZ16: WriteBlock16<aligned>((BYTE*)&m_vm16[BlockAddress16Z(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			case PSM_PSMZ16S: WriteBlock16<aligned>((BYTE*)&m_vm16[BlockAddress16SZ(x, y, bp, bw)], &src[x * 2], srcpitch); break;
			// TODO
			default: __assume(0);
			}
		}
	}
}

template<int psm, int bsx, int bsy>
void GSLocalMemory::WriteImageLeftRight(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	for(; h > 0; y++, h--, src += srcpitch)
	{
		for(int x = l; x < r; x++)
		{
			switch(psm)
			{
			case PSM_PSMCT32: WritePixel32(x, y, *(DWORD*)&src[x * 4], bp, bw); break;
			case PSM_PSMCT16: WritePixel16(x, y, *(WORD*)&src[x * 2], bp, bw); break;
			case PSM_PSMCT16S: WritePixel16S(x, y, *(WORD*)&src[x * 2], bp, bw); break;
			case PSM_PSMT8: WritePixel8(x, y, src[x], bp, bw); break;
			case PSM_PSMT4: WritePixel4(x, y, src[x >> 1] >> ((x & 1) << 2), bp, bw); break;
			case PSM_PSMZ32: WritePixel32Z(x, y, *(DWORD*)&src[x * 4], bp, bw); break;
			case PSM_PSMZ16: WritePixel16Z(x, y, *(WORD*)&src[x * 2], bp, bw); break;
			case PSM_PSMZ16S: WritePixel16SZ(x, y, *(WORD*)&src[x * 2], bp, bw); break;
			// TODO
			default: __assume(0);
			}			
		}
	}
}

template<int psm, int bsx, int bsy, int trbpp>
void GSLocalMemory::WriteImageTopBottom(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	__declspec(align(16)) BYTE buff[64]; // merge buffer for one column

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	const int csy = bsy / 4;

	// merge incomplete column

	int y2 = y & (csy - 1);

	if(y2 > 0)
	{
		int h2 = min(h, csy - y2);

		for(int x = l; x < r; x += bsx)
		{
			BYTE* dst = NULL;

			switch(psm)
			{
			case PSM_PSMCT32: dst = (BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)]; break;
			case PSM_PSMCT16: dst = (BYTE*)&m_vm16[BlockAddress16(x, y, bp, bw)]; break;
			case PSM_PSMCT16S: dst = (BYTE*)&m_vm16[BlockAddress16S(x, y, bp, bw)]; break;
			case PSM_PSMT8: dst = (BYTE*)&m_vm8[BlockAddress8(x, y, bp, bw)]; break;
			case PSM_PSMT4: dst = (BYTE*)&m_vm8[BlockAddress4(x, y, bp, bw) >> 1]; break;
			case PSM_PSMZ32: dst = (BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)]; break;
			case PSM_PSMZ16: dst = (BYTE*)&m_vm16[BlockAddress16Z(x, y, bp, bw)]; break;
			case PSM_PSMZ16S: dst = (BYTE*)&m_vm16[BlockAddress16SZ(x, y, bp, bw)]; break;
			// TODO
			default: __assume(0);
			}

			switch(psm)
			{
			case PSM_PSMCT32:
			case PSM_PSMZ32: 
				ReadColumn32<true>(y, dst, buff, 32);
				memcpy(&buff[32], &src[x * 4], 32);
				WriteColumn32<true, 0xffffffff>(y, dst, buff, 32);
				break;
			case PSM_PSMCT16:
			case PSM_PSMCT16S:
			case PSM_PSMZ16:
			case PSM_PSMZ16S:
				ReadColumn16<true>(y, dst, buff, 32);
				memcpy(&buff[32], &src[x * 2], 32);
				WriteColumn16<true>(y, dst, buff, 32);
				break;
			case PSM_PSMT8: 
				ReadColumn8<true>(y, dst, buff, 16);
				memcpy(&buff[y2 * 16], &src[x], h2 * 16);
				WriteColumn8<true>(y, dst, buff, 16);
				break;
			case PSM_PSMT4: 
				ReadColumn4<true>(y, dst, buff, 16);
				memcpy(&buff[y2 * 16], &src[x >> 1], h2 * 16);
				WriteColumn4<true>(y, dst, buff, 16);
				break;
			// TODO
			default: 
				__assume(0);
			}
		}

		src += srcpitch * h2;
		y += h2;
		h -= h2;
	}

	// write whole columns

	{
		int h2 = h & ~(csy - 1);

		if(h2 > 0)
		{
			if(((DWORD_PTR)&src[l * trbpp >> 3] & 15) == 0 && (srcpitch & 15) == 0)
			{
				WriteImageColumn<psm, bsx, bsy, true>(l, r, y, h2, src, srcpitch, BITBLTBUF);
			}
			else
			{
				WriteImageColumn<psm, bsx, bsy, false>(l, r, y, h2, src, srcpitch, BITBLTBUF);
			}

			src += srcpitch * h2;
			y += h2;
			h -= h2;
		}
	}

	// merge incomplete column

	if(h >= 1)
	{
		for(int x = l; x < r; x += bsx)
		{
			BYTE* dst = NULL;

			switch(psm)
			{
			case PSM_PSMCT32: dst = (BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)]; break;
			case PSM_PSMCT16: dst = (BYTE*)&m_vm16[BlockAddress16(x, y, bp, bw)]; break;
			case PSM_PSMCT16S: dst = (BYTE*)&m_vm16[BlockAddress16S(x, y, bp, bw)]; break;
			case PSM_PSMT8: dst = (BYTE*)&m_vm8[BlockAddress8(x, y, bp, bw)]; break;
			case PSM_PSMT4: dst = (BYTE*)&m_vm8[BlockAddress4(x, y, bp, bw) >> 1]; break;
			case PSM_PSMZ32: dst = (BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)]; break;
			case PSM_PSMZ16: dst = (BYTE*)&m_vm16[BlockAddress16Z(x, y, bp, bw)]; break;
			case PSM_PSMZ16S: dst = (BYTE*)&m_vm16[BlockAddress16SZ(x, y, bp, bw)]; break;
			// TODO
			default: __assume(0);
			}

			switch(psm)
			{
			case PSM_PSMCT32:
			case PSM_PSMZ32: 
				ReadColumn32<true>(y, dst, buff, 32);
				memcpy(&buff[0], &src[x * 4], 32);
				WriteColumn32<true, 0xffffffff>(y, dst, buff, 32);
				break;
			case PSM_PSMCT16:
			case PSM_PSMCT16S:
			case PSM_PSMZ16:
			case PSM_PSMZ16S:
				ReadColumn16<true>(y, dst, buff, 32);
				memcpy(&buff[0], &src[x * 2], 32);
				WriteColumn16<true>(y, dst, buff, 32);
				break;
			case PSM_PSMT8: 
				ReadColumn8<true>(y, dst, buff, 16);
				memcpy(&buff[0], &src[x], h * 16);
				WriteColumn8<true>(y, dst, buff, 16);
				break;
			case PSM_PSMT4: 
				ReadColumn4<true>(y, dst, buff, 16);
				memcpy(&buff[0], &src[x >> 1], h * 16);
				WriteColumn4<true>(y, dst, buff, 16);
				break;
			// TODO
			default: 
				__assume(0);
			}
		}
	}
}

template<int psm, int bsx, int bsy, int trbpp>
void GSLocalMemory::WriteImage(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int l = (int)TRXPOS.DSAX;
	int r = (int)TRXREG.RRW;

	// finish the incomplete row first

	if(tx != l) 
	{
		int n = min(len, (r - tx) * trbpp >> 3);
		WriteImageX(tx, ty, src, n, BITBLTBUF, TRXPOS, TRXREG);
		src += n;
		len -= n;
	}

	int la = (l + (bsx - 1)) & ~(bsx - 1);
	int ra = r & ~(bsx - 1);
	int srcpitch = (r - l) * trbpp >> 3;
	int h = len / srcpitch;

	// transfer width >= block width, and there is at least one full row

	if(ra - la >= bsx && h > 0)
	{
		BYTE* s = &src[-l * trbpp >> 3];

		src += srcpitch * h;
		len -= srcpitch * h;

		// left part

		if(l < la)
		{
			WriteImageLeftRight<psm, bsx, bsy>(l, la, ty, h, s, srcpitch, BITBLTBUF);
		}

		// right part

		if(ra < r)
		{
			WriteImageLeftRight<psm, bsx, bsy>(ra, r, ty, h, s, srcpitch, BITBLTBUF);
		}

		// horizontally aligned part

		if(la < ra)
		{
			// top part

			{
				int h2 = min(h, bsy - (ty & (bsy - 1)));

				if(h2 < bsy)
				{
					WriteImageTopBottom<psm, bsx, bsy, trbpp>(la, ra, ty, h2, s, srcpitch, BITBLTBUF);

					s += srcpitch * h2;
					ty += h2;
					h -= h2;
				}
			}

			// horizontally and vertically aligned part

			{
				int h2 = h & ~(bsy - 1);

				if(h2 > 0)
				{
					if(((DWORD_PTR)&s[la * trbpp >> 3] & 15) == 0 && (srcpitch & 15) == 0)
					{
						WriteImageBlock<psm, bsx, bsy, true>(la, ra, ty, h2, s, srcpitch, BITBLTBUF);
					}
					else
					{
						WriteImageBlock<psm, bsx, bsy, false>(la, ra, ty, h2, s, srcpitch, BITBLTBUF);
					}

					s += srcpitch * h2;
					ty += h2;
					h -= h2;
				}
			}

			// bottom part

			if(h > 0)
			{
				WriteImageTopBottom<psm, bsx, bsy, trbpp>(la, ra, ty, h, s, srcpitch, BITBLTBUF);

				// s += srcpitch * h;
				ty += h;
				// h -= h;
			}
		}
	}

	// the rest

	if(len > 0)
	{
		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
}


#define IsTopLeftAligned(dsax, tx, ty, bw, bh) \
	((((int)dsax) & ((bw)-1)) == 0 && ((tx) & ((bw)-1)) == 0 && ((int)dsax) == (tx) && ((ty) & ((bh)-1)) == 0)

void GSLocalMemory::WriteImage24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX) * 3;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock24(src + (x - tx) * 3, srcpitch, (BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)]);
			}
		}

		ty = th;
	}
}
void GSLocalMemory::WriteImage8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	int tw = TRXREG.RRW, srcpitch = TRXREG.RRW - TRXPOS.DSAX;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock8H(src + (x - tx), srcpitch, (BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)]);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::WriteImage4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX) / 2;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock4HL(src + (x - tx) / 2, srcpitch, (BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)]);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::WriteImage4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX) / 2;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock4HH(src + (x - tx) / 2, srcpitch, (BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)]);
			}
		}

		ty = th;
	}
}
void GSLocalMemory::WriteImage24Z(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX) * 3;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock24(src + (x - tx) * 3, srcpitch, (BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)]);
			}
		}

		ty = th;
	}
}
void GSLocalMemory::WriteImageX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(len <= 0) return;

	// if(ty >= (int)TRXREG.RRH) {ASSERT(0); return;}

	BYTE* pb = (BYTE*)src;
	WORD* pw = (WORD*)src;
	DWORD* pd = (DWORD*)src;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;
	psm_t* psm = &m_psm[BITBLTBUF.DPSM];

	int x = tx;
	int y = ty;
	int sx = (int)TRXPOS.DSAX;
	int ex = (int)TRXREG.RRW;

	switch(BITBLTBUF.DPSM)
	{
	case PSM_PSMCT32:
	case PSM_PSMZ32:

		len /= 4;

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pd++)
			{
				WritePixel32(addr + offset[x], *pd);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT24:
	case PSM_PSMZ24:

		len /= 3;

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb += 3)
			{
				WritePixel24(addr + offset[x], *(DWORD*)pb);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:

		len /= 2;

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pw++)
			{
				WritePixel16(addr + offset[x], *pw);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				WritePixel8(addr + offset[x], *pb);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				WritePixel4(addr + offset[x + 0], *pb & 0xf);
				WritePixel4(addr + offset[x + 1], *pb >> 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8H:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				WritePixel8H(addr + offset[x], *pb);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4HL:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				WritePixel4HL(addr + offset[x + 0], *pb & 0xf);
				WritePixel4HL(addr + offset[x + 1], *pb >> 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4HH:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				WritePixel4HH(addr + offset[x + 0], *pb & 0xf);
				WritePixel4HH(addr + offset[x + 1], *pb >> 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;
	}

	tx = x;
	ty = y;
}

//

void GSLocalMemory::ReadImageX(int& tx, int& ty, BYTE* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const
{
	if(len <= 0) return;

	// if(ty >= (int)TRXREG.RRH) {ASSERT(0); return;}

	BYTE* pb = (BYTE*)dst;
	WORD* pw = (WORD*)dst;
	DWORD* pd = (DWORD*)dst;

	DWORD bp = BITBLTBUF.SBP;
	DWORD bw = BITBLTBUF.SBW;
	psm_t* psm = &m_psm[BITBLTBUF.SPSM];

	int x = tx;
	int y = ty;
	int sx = (int)TRXPOS.SSAX;
	int ex = (int)TRXREG.RRW;

	switch(BITBLTBUF.SPSM)
	{
	case PSM_PSMCT32:
	case PSM_PSMZ32:

		len /= 4;

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pd++)
			{
				*pd = ReadPixel32(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT24:
	case PSM_PSMZ24:

		len /= 3;

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb += 3)
			{
				DWORD dw = ReadPixel32(addr + offset[x]);
				
				pb[0] = ((BYTE*)&dw)[0]; 
				pb[1] = ((BYTE*)&dw)[1]; 
				pb[2] = ((BYTE*)&dw)[2];
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:

		len /= 2;

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pw++)
			{
				*pw = ReadPixel16(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				*pb = ReadPixel8(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				*pb = ReadPixel4(addr + offset[x + 0]) | (ReadPixel4(addr + offset[x + 1]) << 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8H:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				*pb = ReadPixel8H(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4HL:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				*pb = ReadPixel4HL(addr + offset[x + 0]) | (ReadPixel4HL(addr + offset[x + 1]) << 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;
	
	case PSM_PSMT4HH:

		while(len > 0)
		{
			DWORD addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				*pb = ReadPixel4HH(addr + offset[x + 0]) | (ReadPixel4HH(addr + offset[x + 1]) << 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;
	}

	tx = x;
	ty = y;
}

///////////////////

void GSLocalMemory::ReadTexture32(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadBlock32<true>((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END

}

void GSLocalMemory::ReadTexture24(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	if(TEXA.AEM)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<true>((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<false>((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture16(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) WORD block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>((BYTE*)&m_vm16[BlockAddress16(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16S(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) WORD block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>((BYTE*)&m_vm16[BlockAddress16S(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	FOREACH_BLOCK_START(16, 16, 32)
	{
		ReadAndExpandBlock8_32((BYTE*)&m_vm8[BlockAddress8(x, y, bp, bw)], dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const UINT64* pal = m_clut;

	FOREACH_BLOCK_START(32, 16, 32)
	{
		ReadAndExpandBlock4_32((BYTE*)&m_vm8[BlockAddress4(x, y, bp, bw) >> 1], dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8H(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadAndExpandBlock8H_32((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HL(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadAndExpandBlock4HL_32((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HH(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadAndExpandBlock4HH_32((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture32Z(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadBlock32<true>((BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture24Z(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	if(TEXA.AEM)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<true>((BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)], dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<false>((BYTE*)&m_vm32[BlockAddress32Z(x, y, bp, bw)], dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture16Z(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) WORD block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>((BYTE*)&m_vm16[BlockAddress16Z(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16SZ(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) WORD block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>((BYTE*)&m_vm16[BlockAddress16SZ(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

///////////////////

void GSLocalMemory::ReadTexture(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP)
{
	readTexture rtx = m_psm[TEX0.PSM].rtx;
	readTexel rt = m_psm[TEX0.PSM].rt;
	CSize bs = m_psm[TEX0.PSM].bs;

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)) 
	|| (CLAMP.WMS == 3) || (CLAMP.WMT == 3))
	{
		ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, rtx);
	}
	else
	{
		(this->*rtx)(r, dst, dstpitch, TEX0, TEXA);
	}
}

void GSLocalMemory::ReadTextureNC(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP)
{
	readTexture rtx = m_psm[TEX0.PSM].rtx;
	readTexel rt = m_psm[TEX0.PSM].rt;
	CSize bs = m_psm[TEX0.PSM].bs;

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)))
	{
		ReadTextureNC<DWORD>(r, dst, dstpitch, TEX0, TEXA, rt, rtx);
	}
	else
	{
		(this->*rtx)(r, dst, dstpitch, TEX0, TEXA);
	}
}
///////////////////

void GSLocalMemory::ReadTexture16NP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>((BYTE*)&m_vm16[BlockAddress16(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16SNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>((BYTE*)&m_vm16[BlockAddress16S(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8NP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(16, 16, 32)
		{
			ReadAndExpandBlock8_32((BYTE*)&m_vm8[BlockAddress8(x, y, bp, bw)], dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) BYTE block[16 * 16];

		FOREACH_BLOCK_START(16, 16, 16)
		{
			ReadBlock8<true>(&m_vm8[BlockAddress8(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 16);

			ExpandBlock8_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture4NP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const UINT64* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(32, 16, 32)
		{
			ReadAndExpandBlock4_32(&m_vm8[BlockAddress4(x, y, bp, bw) >> 1], dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) BYTE block[(32 / 2) * 16];

		FOREACH_BLOCK_START(32, 16, 16)
		{
			ReadBlock4<true>(&m_vm8[BlockAddress4(x, y, bp, bw)>>1], (BYTE*)block, sizeof(block) / 16);

			ExpandBlock4_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture8HNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock8H_32((const BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) DWORD block[8 * 8];

		FOREACH_BLOCK_START(8, 8, 16)
		{
			ReadBlock32<true>((const BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 8);

			ExpandBlock8H_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture4HLNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock4HL_32((const BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) DWORD block[8 * 8];

		FOREACH_BLOCK_START(8, 8, 16)
		{
			ReadBlock32<true>((const BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 8);

			ExpandBlock4HL_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture4HHNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const DWORD* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock4HH_32((const BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) DWORD block[8 * 8];

		FOREACH_BLOCK_START(8, 8, 16)
		{
			ReadBlock32<true>((const BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block) / 8);

			ExpandBlock4HH_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture16ZNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>((const BYTE*)&m_vm16[BlockAddress16Z(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16SZNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>((const BYTE*)&m_vm16[BlockAddress16SZ(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

///////////////////

void GSLocalMemory::ReadTextureNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP)
{
	readTexture rtx = m_psm[TEX0.PSM].rtxNP;
	readTexel rt = m_psm[TEX0.PSM].rtNP;
	CSize bs = m_psm[TEX0.PSM].bs;

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)) 
	|| (CLAMP.WMS == 3) || (CLAMP.WMT == 3))
	{
		DWORD psm = TEX0.PSM;

		switch(psm)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			psm = TEX0.CPSM;
			break;
		}

		switch(psm)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, rtx);
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			ReadTexture<WORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, rtx);
			break;
		}
	}
	else
	{
		(this->*rtx)(r, dst, dstpitch, TEX0, TEXA);
	}
}

void GSLocalMemory::ReadTextureNPNC(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP)
{
	readTexture rtx = m_psm[TEX0.PSM].rtxNP;
	readTexel rt = m_psm[TEX0.PSM].rtNP;
	CSize bs = m_psm[TEX0.PSM].bs;

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)))
	{
		DWORD psm = TEX0.PSM;

		switch(psm)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			psm = TEX0.CPSM;
			break;
		}

		switch(psm)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			ReadTextureNC<DWORD>(r, dst, dstpitch, TEX0, TEXA, rt, rtx);
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			ReadTextureNC<WORD>(r, dst, dstpitch, TEX0, TEXA, rt, rtx);
			break;
		}
	}
	else
	{
		(this->*rtx)(r, dst, dstpitch, TEX0, TEXA);
	}
}

// 32/8

void GSLocalMemory::ReadTexture8P(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 16, 8)
	{
		ReadBlock8<true>(&m_vm8[BlockAddress8(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4P(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(32, 16, 8)
	{
		ReadBlock4P(&m_vm8[BlockAddress4(x, y, bp, bw) >> 1], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8HP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 8)
	{
		ReadBlock8HP((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HLP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 8)
	{
		ReadBlock4HLP((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HHP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 8)
	{
		ReadBlock4HHP((BYTE*)&m_vm32[BlockAddress32(x, y, bp, bw)], dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

//

template<typename T> 
void GSLocalMemory::ReadTexture(CRect r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP, readTexel rt, readTexture rtx)
{
	// TODO: this is a mess, make it more simple

	DWORD wms = CLAMP.WMS, wmt = CLAMP.WMT;
	DWORD minu = CLAMP.MINU, maxu = CLAMP.MAXU;
	DWORD minv = CLAMP.MINV, maxv = CLAMP.MAXV;

	CSize bs = m_psm[TEX0.PSM].bs;

	int bsxm = bs.cx - 1;
	int bsym = bs.cy - 1;

	if(wms == 3 || wmt == 3)
	{
		if(wms == 3 && wmt == 3)
		{
			int w = minu + 1;
			int h = minv + 1;

			w = (w + bsxm) & ~bsxm;
			h = (h + bsym) & ~bsym;

			if(w % bs.cx == 0 && maxu % bs.cx == 0 && h % bs.cy == 0 && maxv % bs.cy == 0)
			{
// printf("!!! 1 wms = %d, wmt = %d, %3x %3x %3x %3x, %d %d - %d %d\n", wms, wmt, minu, maxu, minv, maxv, r.left, r.top, r.right, r.bottom);

				T* buff = (T*)_aligned_malloc(w * h * sizeof(T), 16);

				(this->*rtx)(CRect(CPoint(maxu, maxv), CSize(w, h)), (BYTE*)buff, w * sizeof(T), TEX0, TEXA);

				dst -= r.left * sizeof(T);

//				int left = (r.left + minu) & ~minu;
//				int right = r.right & ~minu;

				for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
				{
					T* src = &buff[(y & minv) * w];

					int x = r.left;
/*
					for(; x < left; x++)
					{
						((T*)dst)[x] = src[x & minu];
					}

					for(; x < right; x += minu + 1)
					{
						memcpy(&((T*)dst)[x], src, sizeof(T) * (minu + 1));
					}
*/
					for(; x < r.right; x++)
					{
						((T*)dst)[x] = src[x & minu];
					}
				}

				_aligned_free(buff);

				return;
			}
		}

		if(wms == 2)
		{
			int left = r.left;
			r.left = min(r.right, max(r.left, (int)minu));
			r.right = max(r.left, min(r.right, (int)maxu + 1));
			dst += (r.left - left) * sizeof(T);
		}

		if(wmt == 2)
		{
			int top = r.top;
			r.top = min(r.bottom, max(r.top, (int)minv));
			r.bottom = max(r.top, min(r.bottom, (int)maxv + 1));
			dst += (r.top - top) * dstpitch;
		}

		if(wms == 3 && wmt != 3)
		{
			int w = ((minu + 1) + bsxm) & ~bsxm;

			if(w % bs.cx == 0 && maxu % bs.cx == 0)
			{
// printf("!!! 2 wms = %d, wmt = %d, %3x %3x %3x %3x, %d %d - %d %d\n", wms, wmt, minu, maxu, minv, maxv, r.left, r.top, r.right, r.bottom);
				int top = r.top & ~bsym; 
				int bottom = (r.bottom + bsym) & ~bsym;
				
				int h = bottom - top;

				T* buff = (T*)_aligned_malloc(w * h * sizeof(T), 16);

				(this->*rtx)(CRect(CPoint(maxu, top), CSize(w, h)), (BYTE*)buff, w * sizeof(T), TEX0, TEXA);

				dst -= r.left * sizeof(T);

//				int left = (r.left + minu) & ~minu;
//				int right = r.right & ~minu;

				for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
				{
					T* src = &buff[(y - top) * w];

					int x = r.left;
/*
					for(; x < left; x++)
					{
						((T*)dst)[x] = src[x & minu];
					}

					for(; x < right; x += minu + 1)
					{
						memcpy(&((T*)dst)[x], src, sizeof(T) * (minu + 1));
					}
*/
					for(; x < r.right; x++)
					{
						((T*)dst)[x] = src[x & minu];
					}
				}

				_aligned_free(buff);

				return;
			}
		}

		if(wms != 3 && wmt == 3)
		{
			int h = (minv + 1 + bsym) & ~bsym;

			if(h % bs.cy == 0 && maxv % bs.cy == 0)
			{
// printf("!!! 3 wms = %d, wmt = %d, %3x %3x %3x %3x, %d %d - %d %d\n", wms, wmt, minu, maxu, minv, maxv, r.left, r.top, r.right, r.bottom);
				int left = r.left & ~bsxm; 
				int right = (r.right + bsxm) & ~bsxm;
				
				int w = right - left;

				T* buff = (T*)_aligned_malloc(w * h * sizeof(T), 16);

				(this->*rtx)(CRect(CPoint(left, maxv), CSize(w, h)), (BYTE*)buff, w * sizeof(T), TEX0, TEXA);

				for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
				{
					T* src = &buff[(y & minv) * w + (r.left - left)];

					memcpy(dst, src, sizeof(T) * r.Width());
				}

				_aligned_free(buff);

				return;
			}
		}

		switch(wms)
		{
		default: for(int x = r.left; x < r.right; x++) m_xtbl[x] = x; break;
		case 3: for(int x = r.left; x < r.right; x++) m_xtbl[x] = (x & minu) | maxu; break;
		}

		switch(wmt)
		{
		default: for(int y = r.top; y < r.bottom; y++) m_ytbl[y] = y; break;
		case 3: for(int y = r.top; y < r.bottom; y++) m_ytbl[y] = (y & minv) | maxv; break;
		}

// printf("!!! 4 wms = %d, wmt = %d, %3x %3x %3x %3x, %d %d - %d %d\n", wms, wmt, minu, maxu, minv, maxv, r.left, r.top, r.right, r.bottom);

		for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
			for(int x = r.left, i = 0; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(m_xtbl[x], m_ytbl[y], TEX0, TEXA);
	}
	else
	{
		// find a block-aligned rect that fits between r and the region clamped area (if any)

		CRect r1 = r;
		CRect r2 = r;

		r1.left = (r1.left + bsxm) & ~bsxm;
		r1.top = (r1.top + bsym) & ~bsym;
		r1.right = r1.right & ~bsxm; 
		r1.bottom = r1.bottom & ~bsym; 

		if(wms == 2 && minu < maxu) 
		{
			r2.left = minu & ~bsxm; 
			r2.right = (maxu + bsxm) & ~bsxm;
		}

		if(wmt == 2 && minv < maxv) 
		{
			r2.top = minv & ~bsym; 
			r2.bottom = (maxv + bsym) & ~bsym;
		}

		CRect cr = r1 & r2;

		bool aligned = ((DWORD_PTR)(dst + (cr.left - r.left) * sizeof(T)) & 0xf) == 0;

		if(cr.left >= cr.right && cr.top >= cr.bottom || !aligned)
		{
			// TODO: expand r to block size, read into temp buffer, copy to r (like above)

if(!aligned) printf("unaligned memory pointer passed to ReadTexture\n");

// printf("!!! 5 wms = %d, wmt = %d, %3x %3x %3x %3x, %d %d - %d %d\n", wms, wmt, minu, maxu, minv, maxv, r.left, r.top, r.right, r.bottom);

			for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
		}
		else
		{
// printf("!!! 6 wms = %d, wmt = %d, %3x %3x %3x %3x, %d %d - %d %d\n", wms, wmt, minu, maxu, minv, maxv, r.left, r.top, r.right, r.bottom);

			for(int y = r.top; y < cr.top; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);

			if(!cr.IsRectEmpty())
			{
				(this->*rtx)(cr, dst + (cr.left - r.left) * sizeof(T), dstpitch, TEX0, TEXA);
			}

			for(int y = cr.top; y < cr.bottom; y++, dst += dstpitch)
			{
				for(int x = r.left, i = 0; x < cr.left; x++, i++)
					((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
				for(int x = cr.right, i = x - r.left; x < r.right; x++, i++)
					((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
			}

			for(int y = cr.bottom; y < r.bottom; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
		}
	}
}

template<typename T> 
void GSLocalMemory::ReadTextureNC(CRect r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, readTexel rt, readTexture rtx)
{
	CSize bs = m_psm[TEX0.PSM].bs;

	int bsxm = bs.cx - 1;
	int bsym = bs.cy - 1;

	CRect cr;

	cr.left = (r.left + bsxm) & ~bsxm;
	cr.top = (r.top + bsym) & ~bsym;
	cr.right = r.right & ~bsxm; 
	cr.bottom = r.bottom & ~bsym; 

	bool aligned = ((DWORD_PTR)(dst + (cr.left - r.left) * sizeof(T)) & 0xf) == 0;

	if(cr.left >= cr.right && cr.top >= cr.bottom || !aligned)
	{
		// TODO: expand r to block size, read into temp buffer, copy to r (like above)

if(!aligned) printf("unaligned memory pointer passed to ReadTexture\n");

		for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
			for(int x = r.left, i = 0; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
	}
	else
	{
		for(int y = r.top; y < cr.top; y++, dst += dstpitch)
			for(int x = r.left, i = 0; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);

		if(!cr.IsRectEmpty())
			(this->*rtx)(cr, dst + (cr.left - r.left) * sizeof(T), dstpitch, TEX0, TEXA);

		for(int y = cr.top; y < cr.bottom; y++, dst += dstpitch)
		{
			for(int x = r.left, i = 0; x < cr.left; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
			for(int x = cr.right, i = x - r.left; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
		}

		for(int y = cr.bottom; y < r.bottom; y++, dst += dstpitch)
			for(int x = r.left, i = 0; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
	}
}

HRESULT GSLocalMemory::SaveBMP(LPCTSTR fn, DWORD bp, DWORD bw, DWORD psm, int w, int h)
{
	int pitch = w * 4;
	int size = pitch * h;
	void* bits = ::_aligned_malloc(size, 16);

	GIFRegTEX0 TEX0;

	TEX0.TBP0 = bp;
	TEX0.TBW = bw;
	TEX0.PSM = psm;

	GIFRegTEXA TEXA;

	TEXA.AEM = 0;
	TEXA.TA0 = 0;
	TEXA.TA1 = 0x80;

	// (this->*m_psm[TEX0.PSM].rtx)(CRect(0, 0, w, h), bits, pitch, TEX0, TEXA);

	readPixel rp = m_psm[psm].rp;

	BYTE* p = (BYTE*)bits;

	for(int j = h-1; j >= 0; j--, p += pitch)
		for(int i = 0; i < w; i++)
			((DWORD*)p)[i] = (this->*rp)(i, j, TEX0.TBP0, TEX0.TBW);

	if(FILE* fp = _tfopen(fn, _T("wb")))
	{
		BITMAPINFOHEADER bih;
		memset(&bih, 0, sizeof(bih));
        bih.biSize = sizeof(bih);
        bih.biWidth = w;
        bih.biHeight = h;
        bih.biPlanes = 1;
        bih.biBitCount = 32;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = size;

		BITMAPFILEHEADER bfh;
		memset(&bfh, 0, sizeof(bfh));
		bfh.bfType = 'MB';
		bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
		bfh.bfSize = bfh.bfOffBits + size;
		bfh.bfReserved1 = bfh.bfReserved2 = 0;

		fwrite(&bfh, 1, sizeof(bfh), fp);
		fwrite(&bih, 1, sizeof(bih), fp);
		fwrite(bits, 1, size, fp);

		fclose(fp);
	}

	::_aligned_free(bits);

	return true;
}
