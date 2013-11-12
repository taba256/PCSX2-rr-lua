/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __VIF_H__
#define __VIF_H__

struct vifCycle {
	u8 cl, wl;
	u8 pad[2];
};

struct VIFregisters {
	u32 stat;
	u32 pad0[3];
	u32 fbrst;
	u32 pad1[3];
	u32 err;
	u32 pad2[3];
	u32 mark;
	u32 pad3[3];
	vifCycle cycle; //data write cycle
	u32 pad4[3];
	u32 mode;
	u32 pad5[3];
	u32 num;
	u32 pad6[3];
	u32 mask;
	u32 pad7[3];
	u32 code;
	u32 pad8[3];
	u32 itops;
	u32 pad9[3];
	u32 base;      // Not used in VIF0
	u32 pad10[3];
	u32 ofst;      // Not used in VIF0
	u32 pad11[3];
	u32 tops;      // Not used in VIF0
	u32 pad12[3];
	u32 itop;
	u32 pad13[3];
	u32 top;       // Not used in VIF0
	u32 pad14[3];
	u32 mskpath3;
	u32 pad15[3];
	u32 r0;        // row0 register
	u32 pad16[3];
	u32 r1;        // row1 register
	u32 pad17[3];
	u32 r2;        // row2 register
	u32 pad18[3];
	u32 r3;        // row3 register
	u32 pad19[3];
	u32 c0;        // col0 register
	u32 pad20[3];
	u32 c1;        // col1 register
	u32 pad21[3];
	u32 c2;        // col2 register
	u32 pad22[3];
	u32 c3;        // col3 register
	u32 pad23[3];
	u32 offset;    // internal UNPACK offset
	u32 addr;
};

extern "C"
{
	// these use cdecl for Asm code references.
	extern VIFregisters *_vifRegs;
	extern u32* _vifMaskRegs;
	extern u32* _vifRow;
	extern u32* _vifCol;
}

#define vif0Regs ((VIFregisters*)&PS2MEM_HW[0x3800])
#define vif1Regs ((VIFregisters*)&PS2MEM_HW[0x3c00])

void dmaVIF0();
void dmaVIF1();
void mfifoVIF1transfer(int qwc);
int  VIF0transfer(u32 *data, int size, int istag);
int  VIF1transfer(u32 *data, int size, int istag);
void  vifMFIFOInterrupt();

void __fastcall SetNewMask(u32* vif1masks, u32* hasmask, u32 mask, u32 oldmask);

#define XMM_R0			xmm0
#define XMM_R1			xmm1
#define XMM_R2			xmm2
#define XMM_WRITEMASK	xmm3
#define XMM_ROWMASK		xmm4
#define XMM_ROWCOLMASK	xmm5
#define XMM_ROW			xmm6
#define XMM_COL			xmm7

#define XMM_R3			XMM_COL


#endif /* __VIF_H__ */
