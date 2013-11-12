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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "IopMem.h"

// The full suite of hardware APIs:
#include "IPU/IPU.h"
#include "GS.h"
#include "Counters.h"
#include "Vif.h"
#include "VifDma.h"
#include "SPR.h"
#include "Sif.h"

using namespace R5900;

/////////////////////////////////////////////////////////////////////////
// DMA Execution Interfaces

// dark cloud2 uses 8 bit DMAs register writes
static __forceinline void DmaExec8( void (*func)(), u32 mem, u8 value )
{
	//Its invalid for the hardware to write a DMA while it is active, not without Suspending the DMAC
	if((value & 0x1) && (psHu8(mem) & 0x1) == 0x1 && (psHu32(DMAC_CTRL) & 0x1) == 1) {
		DMA_LOG( "DMAExec8 Attempt to run DMA while one is already active mem = %x", mem );
		return;
	}
	psHu8(mem) = (u8)value;
	if ((psHu8(mem) & 0x1) && (psHu32(DMAC_CTRL) & 0x1))
	{
		/*SysPrintf("Running DMA 8 %x\n", psHu32(mem & ~0x1));*/
		func();
	}
}

static __forceinline void DmaExec16( void (*func)(), u32 mem, u16 value )
{
	//Its invalid for the hardware to write a DMA while it is active, not without Suspending the DMAC
	if((value & 0x100) && (psHu32(mem) & 0x100) == 0x100 && (psHu32(DMAC_CTRL) & 0x1) == 1) {
		DMA_LOG( "DMAExec16 Attempt to run DMA while one is already active mem = %x", mem);
		return;
	}
	psHu16(mem) = (u16)value;
	if ((psHu16(mem) & 0x100) && (psHu32(DMAC_CTRL) & 0x1))
	{
		//SysPrintf("16bit DMA Start\n");
		func();
	}
}

static void DmaExec( void (*func)(), u32 mem, u32 value )
{
	//Its invalid for the hardware to write a DMA while it is active, not without Suspending the DMAC
	if((value & 0x100) && (psHu32(mem) & 0x100) == 0x100 && (psHu32(DMAC_CTRL) & 0x1) == 1) {
		DMA_LOG( "DMAExec32 Attempt to run DMA while one is already active mem = %x", mem );
		return;
	}
	/* Keep the old tag if in chain mode and hw doesnt set it*/
	if( (value & 0xc) == 0x4 && (value & 0xffff0000) == 0)
		psHu32(mem) = (psHu32(mem) & 0xFFFF0000) | (u16)value;
	else /* Else (including Normal mode etc) write whatever the hardware sends*/
		psHu32(mem) = (u32)value;

	if ((psHu32(mem) & 0x100) && (psHu32(DMAC_CTRL) & 0x1))
		func();
}


/////////////////////////////////////////////////////////////////////////
// Hardware WRITE 8 bit

char sio_buffer[1024];
int sio_count;

void hwWrite8(u32 mem, u8 value) {

	if( (mem>=0x10003800) && (mem<0x10004000) )
	{
		u32 bytemod = mem & 0x3;
		u32 bitpos = 8 * bytemod;
		u32 newval = psHu8(mem) & (255UL << bitpos);
		if( mem < 0x10003c00 )
			vif0Write32( mem & ~0x3, newval | (value<<bitpos));
		else
			vif1Write32( mem & ~0x3, newval | (value<<bitpos));
			
		return;
	}

	if( mem >= 0x10002000 && mem < 0x10008000 )
		DevCon::Notice( "hwWrite8 to 0x%x = 0x%x", params mem, value );

	switch (mem) {
		case 0x10000000: rcntWcount(0, value); break;
		case 0x10000010: rcntWmode(0, (counters[0].modeval & 0xff00) | value); break;
		case 0x10000011: rcntWmode(0, (counters[0].modeval & 0xff) | value << 8); break;
		case 0x10000020: rcntWtarget(0, value); break;
		case 0x10000030: rcntWhold(0, value); break;

		case 0x10000800: rcntWcount(1, value); break;
		case 0x10000810: rcntWmode(1, (counters[1].modeval & 0xff00) | value); break;
		case 0x10000811: rcntWmode(1, (counters[1].modeval & 0xff) | value << 8); break;
		case 0x10000820: rcntWtarget(1, value); break;
		case 0x10000830: rcntWhold(1, value); break;

		case 0x10001000: rcntWcount(2, value); break;
		case 0x10001010: rcntWmode(2, (counters[2].modeval & 0xff00) | value); break;
		case 0x10001011: rcntWmode(2, (counters[2].modeval & 0xff) | value << 8); break;
		case 0x10001020: rcntWtarget(2, value); break;

		case 0x10001800: rcntWcount(3, value); break;
		case 0x10001810: rcntWmode(3, (counters[3].modeval & 0xff00) | value); break;
		case 0x10001811: rcntWmode(3, (counters[3].modeval & 0xff) | value << 8); break;
		case 0x10001820: rcntWtarget(3, value); break;

		case 0x1000f180:
			if (value == '\n') {
				sio_buffer[sio_count] = 0;
				Console::WriteLn( Color_Cyan, sio_buffer );
				sio_count = 0;
			} else {
				if (sio_count < 1023) {
					sio_buffer[sio_count++] = value;
				}
			}
			break;
		
		//case 0x10003c02: //Tony Hawks Project 8 uses this
		//	vif1Write32(mem & ~0x2, value << 16);
		//	break;
		case 0x10008001: // dma0 - vif0
			DMA_LOG("VIF0dma EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaVIF0, mem, value);
			break;

		case 0x10009001: // dma1 - vif1
			DMA_LOG("VIF1dma EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaVIF1, mem, value);
			break;

		case 0x1000a001: // dma2 - gif
			DMA_LOG("GSdma EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaGIF, mem, value);
			break;

		case 0x1000b001: // dma3 - fromIPU
			DMA_LOG("IPU0dma EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaIPU0, mem, value);
			break;

		case 0x1000b401: // dma4 - toIPU
			DMA_LOG("IPU1dma EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaIPU1, mem, value);
			break;

		case 0x1000c001: // dma5 - sif0
			DMA_LOG("SIF0dma EXECUTE, value=0x%x\n", value);
//			if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec8(dmaSIF0, mem, value);
			break;

		case 0x1000c401: // dma6 - sif1
			DMA_LOG("SIF1dma EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaSIF1, mem, value);
			break;

		case 0x1000c801: // dma7 - sif2
			DMA_LOG("SIF2dma EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaSIF2, mem, value);
			break;

		case 0x1000d001: // dma8 - fromSPR
			DMA_LOG("fromSPRdma8 EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaSPR0, mem, value);
			break;

		case 0x1000d401: // dma9 - toSPR
			DMA_LOG("toSPRdma8 EXECUTE, value=0x%x\n", value);
			DmaExec8(dmaSPR1, mem, value);
			break;

		case 0x1000f592: // DMAC_ENABLEW
			psHu8(0xf592) = value;
			psHu8(0xf522) = value;
			break;

		case 0x1000f200: // SIF(?)
			psHu8(mem) = value;
			break;

		case 0x1000f240:// SIF(?)
			if(!(value & 0x100))
				psHu32(mem) &= ~0x100;
			break;
		
		default:
			assert( (mem&0xff0f) != 0xf200 );

			switch(mem&~3) {
				case 0x1000f130:
				case 0x1000f410:
				case 0x1000f430:
					break;
				default:
					psHu8(mem) = value;
			}
			HW_LOG("Unknown Hardware write 8 at %x with value %x\n", mem, value);
			break;
	}
}

__forceinline void hwWrite16(u32 mem, u16 value)
{
	if( mem >= 0x10002000 && mem < 0x10008000 )
		Console::Notice( "hwWrite16 to %x", params mem );

	switch(mem)
	{
		case 0x10000000: rcntWcount(0, value); break;
		case 0x10000010: rcntWmode(0, value); break;
		case 0x10000020: rcntWtarget(0, value); break;
		case 0x10000030: rcntWhold(0, value); break;

		case 0x10000800: rcntWcount(1, value); break;
		case 0x10000810: rcntWmode(1, value); break;
		case 0x10000820: rcntWtarget(1, value); break;
		case 0x10000830: rcntWhold(1, value); break;

		case 0x10001000: rcntWcount(2, value); break;
		case 0x10001010: rcntWmode(2, value); break;
		case 0x10001020: rcntWtarget(2, value); break;

		case 0x10001800: rcntWcount(3, value); break;
		case 0x10001810: rcntWmode(3, value); break;
		case 0x10001820: rcntWtarget(3, value); break;

		case 0x10008000: // dma0 - vif0
			DMA_LOG("VIF0dma %lx\n", value);
			DmaExec16(dmaVIF0, mem, value);
			break;

		case 0x10009000: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma CHCR %lx\n", value);
			DmaExec16(dmaVIF1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x10009010: // dma1 - vif1 - madr
			HW_LOG("VIF1dma Madr %lx\n", value);
			psHu16(mem) = value;//dma1 madr
			break;
		case 0x10009020: // dma1 - vif1 - qwc
			HW_LOG("VIF1dma QWC %lx\n", value);
			psHu16(mem) = value;//dma1 qwc
			break;
		case 0x10009030: // dma1 - vif1 - tadr
			HW_LOG("VIF1dma TADR %lx\n", value);
			psHu16(mem) = value;//dma1 tadr
			break;
		case 0x10009040: // dma1 - vif1 - asr0
			HW_LOG("VIF1dma ASR0 %lx\n", value);
			psHu16(mem) = value;//dma1 asr0
			break;
		case 0x10009050: // dma1 - vif1 - asr1
			HW_LOG("VIF1dma ASR1 %lx\n", value);
			psHu16(mem) = value;//dma1 asr1
			break;
		case 0x10009080: // dma1 - vif1 - sadr
			HW_LOG("VIF1dma SADR %lx\n", value);
			psHu16(mem) = value;//dma1 sadr
			break;
#endif
// ---------------------------------------------------

		case 0x1000a000: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite32: GSdma %lx\n", cpuRegs.cycle, value);
			DmaExec16(dmaGIF, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000a010:
		    psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write DMA2_MADR 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a020:
            psHu16(mem) = value;//dma2 qwc
		    HW_LOG("Hardware write DMA2_QWC 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a030:
            psHu16(mem) = value;//dma2 taddr
		    HW_LOG("Hardware write DMA2_TADDR 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a040:
            psHu16(mem) = value;//dma2 asr0
		    HW_LOG("Hardware write DMA2_ASR0 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a050:
            psHu16(mem) = value;//dma2 asr1
		    HW_LOG("Hardware write DMA2_ASR1 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a080:
            psHu16(mem) = value;//dma2 saddr
		    HW_LOG("Hardware write DMA2_SADDR 32bit at %x with value %x\n",mem,value);
		    break;
#endif

		case 0x1000b000: // dma3 - fromIPU
			DMA_LOG("IPU0dma %lx\n", value);
			DmaExec16(dmaIPU0, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000b010:
	   		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_MADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b020:
    		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_QWC 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b030:
			psHu16(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU0DMA_TADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b080:
			psHu16(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU0DMA_SADDR 32bit at %x with value %x\n",mem,value);
			break;
#endif

		case 0x1000b400: // dma4 - toIPU
			DMA_LOG("IPU1dma %lx\n", value);
			DmaExec16(dmaIPU1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000b410:
    		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_MADR 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b420:
    		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_QWC 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b430:
			psHu16(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU1DMA_TADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b480:
			psHu16(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU1DMA_SADDR 32bit at %x with value %x\n",mem,value);
			break;
#endif
		case 0x1000c000: // dma5 - sif0
			DMA_LOG("SIF0dma %lx\n", value);
//			if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec16(dmaSIF0, mem, value);
			break;

		case 0x1000c002:
			//?
			break;
		case 0x1000c400: // dma6 - sif1
			DMA_LOG("SIF1dma %lx\n", value);
			DmaExec16(dmaSIF1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000c420: // dma6 - sif1 - qwc
			HW_LOG("SIF1dma QWC = %lx\n", value);
			psHu16(mem) = value;
			break;

		case 0x1000c430: // dma6 - sif1 - tadr
			HW_LOG("SIF1dma TADR = %lx\n", value);
			psHu16(mem) = value;
			break;
#endif

		case 0x1000c800: // dma7 - sif2
			DMA_LOG("SIF2dma %lx\n", value);
			DmaExec16(dmaSIF2, mem, value);
			break;
		case 0x1000c802:
			//?
			break;
		case 0x1000d000: // dma8 - fromSPR
			DMA_LOG("fromSPRdma %lx\n", value);
			DmaExec16(dmaSPR0, mem, value);
			break;

		case 0x1000d400: // dma9 - toSPR
			DMA_LOG("toSPRdma %lx\n", value);
			DmaExec16(dmaSPR1, mem, value);
			break;
		case 0x1000f592: // DMAC_ENABLEW
			psHu16(0xf592) = value;
			psHu16(0xf522) = value;
			break;
		case 0x1000f130:
		case 0x1000f132:
		case 0x1000f410:
		case 0x1000f412:
		case 0x1000f430:
		case 0x1000f432:
			break;

		case 0x1000f200:
			psHu16(mem) = value;
			break;

		case 0x1000f220:
			psHu16(mem) |= value;
			break;
		case 0x1000f230:
			psHu16(mem) &= ~value;
			break;
		case 0x1000f240:
			if(!(value & 0x100))
				psHu16(mem) &= ~0x100;
			else
				psHu16(mem) |= 0x100;
			break;
		case 0x1000f260:
			psHu16(mem) = 0;
			break;

		default:
			psHu16(mem) = value;
			HW_LOG("Unknown Hardware write 16 at %x with value %x\n",mem,value);
	}
}

// Page 0 of HW memory houses registers for Counters 0 and 1
void __fastcall hwWrite32_page_00( u32 mem, u32 value )
{
	mem &= 0xffff;
	switch (mem)
	{
		case 0x000: rcntWcount(0, value); return;
		case 0x010: rcntWmode(0, value); return;
		case 0x020: rcntWtarget(0, value); return;
		case 0x030: rcntWhold(0, value); return;

		case 0x800: rcntWcount(1, value); return;
		case 0x810: rcntWmode(1, value); return;
		case 0x820: rcntWtarget(1, value); return;
		case 0x830: rcntWhold(1, value); return;
	}

	*((u32*)&PS2MEM_HW[mem]) = value;
}

// Page 1 of HW memory houses registers for Counters 2 and 3
void __fastcall hwWrite32_page_01( u32 mem, u32 value )
{
	mem &= 0xffff;
	switch (mem)
	{
		case 0x1000: rcntWcount(2, value); return;
		case 0x1010: rcntWmode(2, value); return;
		case 0x1020: rcntWtarget(2, value); return;

		case 0x1800: rcntWcount(3, value); return;
		case 0x1810: rcntWmode(3, value); return;
		case 0x1820: rcntWtarget(3, value); return;
	}

	*((u32*)&PS2MEM_HW[mem]) = value;
}

// page 2 is the IPU register space!
void __fastcall hwWrite32_page_02( u32 mem, u32 value )
{
	ipuWrite32(mem, value);
}

// Page 3 contains writes to vif0 and vif1 registers, plus some GIF stuff!
void __fastcall hwWrite32_page_03( u32 mem, u32 value )
{
	if(mem>=0x10003800)
	{
		if(mem<0x10003c00)
			vif0Write32(mem, value); 
		else
			vif1Write32(mem, value); 
		return;
	}

	switch (mem)
	{
		case GIF_CTRL:
			psHu32(mem) = value & 0x8;
			if (value & 0x1)
				gsGIFReset();
			else if( value & 8 )
				psHu32(GIF_STAT) |= 8;
			else
				psHu32(GIF_STAT) &= ~8;
		break;

		case GIF_MODE:
		{
			// need to set GIF_MODE (hamster ball)
			psHu32(GIF_MODE) = value;

			// set/clear bits 0 and 2 as per the GIF_MODE value.
			const u32 bitmask = 0x1 | 0x4;
			psHu32(GIF_STAT) &= ~bitmask;
			psHu32(GIF_STAT) |= (u32)value & bitmask;
		}
		break;

		case GIF_STAT: // stat is readonly
			DevCon::Notice("*PCSX2* GIFSTAT write value = 0x%x (readonly, ignored)", params value);
		break;

		default:
			psHu32(mem) = value;
	}
}

void __fastcall hwWrite32_page_0B( u32 mem, u32 value )
{
	// Used for developer logging -- optimized away in Public Release.
	const char* regName = "Unknown";

	switch( mem )
	{
		case D3_CHCR: // dma3 - fromIPU
			DMA_LOG("IPU0dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaIPU0, mem, value);
		return;

		case D3_MADR: regName = "IPU0DMA_MADR"; break;
		case D3_QWC: regName = "IPU0DMA_QWC"; break;
		case D3_TADR: regName = "IPU0DMA_TADR"; break;
		case D3_SADR: regName = "IPU0DMA_SADDR"; break;

		//------------------------------------------------------------------

		case D4_CHCR: // dma4 - toIPU
			DMA_LOG("IPU1dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaIPU1, mem, value);
		return;

		case D4_MADR: regName = "IPU1DMA_MADR"; break;
		case D4_QWC: regName = "IPU1DMA_QWC"; break;
		case D4_TADR: regName = "IPU1DMA_TADR"; break;
		case D4_SADR: regName = "IPU1DMA_SADDR"; break;
	}

	HW_LOG( "Hardware Write32 at 0x%x (%s), value=0x%x\n", mem, regName, value );
	psHu32(mem) = value;
}

void __fastcall hwWrite32_page_0E( u32 mem, u32 value )
{
	if( mem == DMAC_CTRL )
	{
		HW_LOG("DMAC_CTRL Write 32bit %x\n", value);
	}
	else if( mem == DMAC_STAT )
	{
		HW_LOG("DMAC_STAT Write 32bit %x\n", value);

		// lower 16 bits: clear on 1
		// upper 16 bits: reverse on 1

		psHu16(0xe010) &= ~(value & 0xffff);
		psHu16(0xe012) ^= (u16)(value >> 16);

		cpuTestDMACInts();
		return;
	}

	psHu32(mem) = value;
}

void __fastcall hwWrite32_page_0F( u32 mem, u32 value )
{
	// Shift the middle 8 bits (bits 4-12) into the lower 8 bits.
	// This helps the compiler optimize the switch statement into a lookup table. :)

#define HELPSWITCH(m) (((m)>>4) & 0xff)

	switch( HELPSWITCH(mem) )
	{
		case HELPSWITCH(INTC_STAT):
			HW_LOG("INTC_STAT Write 32bit %x\n", value);
			psHu32(INTC_STAT) &= ~value;	
			//cpuTestINTCInts();
			break;

		case HELPSWITCH(INTC_MASK):
			HW_LOG("INTC_MASK Write 32bit %x\n", value);
			psHu32(INTC_MASK) ^= (u16)value;
			cpuTestINTCInts();
			break;

		//------------------------------------------------------------------			
		case HELPSWITCH(0x1000f430)://MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
			if ((((value >> 16) & 0xFFF) == 0x21) && (((value >> 6) & 0xF) == 1) && (((psHu32(0xf440) >> 7) & 1) == 0))//INIT & SRP=0
				rdram_sdevid = 0;	// if SIO repeater is cleared, reset sdevid
			psHu32(mem) = value & ~0x80000000;	//kill the busy bit
			break;

		case HELPSWITCH(0x1000f200):
			psHu32(mem) = value;
			break;
		case HELPSWITCH(0x1000f220):
			psHu32(mem) |= value;
			break;
		case HELPSWITCH(0x1000f230):
			psHu32(mem) &= ~value;
			break;
		case HELPSWITCH(0x1000f240):
			if(!(value & 0x100))
				psHu32(mem) &= ~0x100;
			else
				psHu32(mem) |= 0x100;
			break;
		case HELPSWITCH(0x1000f260):
			psHu32(mem) = 0;
			break;

		case HELPSWITCH(0x1000f440)://MCH_DRD:
			psHu32(mem) = value;
			break;

		case HELPSWITCH(0x1000f590): // DMAC_ENABLEW
			HW_LOG("DMAC_ENABLEW Write 32bit %lx\n", value);
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
			break;

		//------------------------------------------------------------------
		case HELPSWITCH(0x1000f130):
		case HELPSWITCH(0x1000f410):
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)\n", mem, value, cpuRegs.CP0.n.Status.val);
		break;

		default:
			psHu32(mem) = value;
	}
}

void __fastcall hwWrite32_generic( u32 mem, u32 value )
{
	// Used for developer logging -- optimized away in Public Release.
	const char* regName = "Unknown";

	switch (mem)
	{
		case D0_CHCR: // dma0 - vif0
			DMA_LOG("VIF0dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaVIF0, mem, value);
			return;

//------------------------------------------------------------------
		case D1_CHCR: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaVIF1, mem, value);
			return;

		case D1_MADR: regName = "VIF1dma MADR"; break;
		case D1_QWC: regName = "VIF1dma QWC"; break;
		case D1_TADR: regName = "VIF1dma TADR"; break;
		case D1_ASR0: regName = "VIF1dma ASR0"; break;
		case D1_ASR1: regName = "VIF1dma ASR1"; break;
		case D1_SADR: regName = "VIF1dma SADR"; break;

//------------------------------------------------------------------
		case D2_CHCR: // dma2 - gif
			DMA_LOG("GIFdma EXECUTE, value=0x%x", value);
			DmaExec(dmaGIF, mem, value);
			return;

		case D2_MADR: regName = "GIFdma MADR"; break;
	    case D2_QWC: regName = "GIFdma QWC"; break;
	    case D2_TADR: regName = "GIFdma TADDR"; break;
	    case D2_ASR0: regName = "GIFdma ASR0"; break;
	    case D2_ASR1: regName = "GIFdma ASR1"; break;
	    case D2_SADR: regName = "GIFdma SADDR"; break;

//------------------------------------------------------------------
		case 0x1000c000: // dma5 - sif0
			DMA_LOG("SIF0dma EXECUTE, value=0x%x\n", value);
			//if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec(dmaSIF0, mem, value);
			return;
//------------------------------------------------------------------
		case 0x1000c400: // dma6 - sif1
			DMA_LOG("SIF1dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaSIF1, mem, value);
			return;

		case 0x1000c420: regName = "SIF1dma QWC"; break;
		case 0x1000c430: regName = "SIF1dma TADR"; break;

//------------------------------------------------------------------
		case 0x1000c800: // dma7 - sif2
			DMA_LOG("SIF2dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaSIF2, mem, value);
			return;
//------------------------------------------------------------------
		case 0x1000d000: // dma8 - fromSPR
			DMA_LOG("SPR0dma EXECUTE (fromSPR), value=0x%x\n", value);
			DmaExec(dmaSPR0, mem, value);
			return;
//------------------------------------------------------------------
		case 0x1000d400: // dma9 - toSPR
			DMA_LOG("SPR1dma EXECUTE (toSPR), value=0x%x\n", value);
			DmaExec(dmaSPR1, mem, value);
			return;
	}
	HW_LOG( "Hardware Write32 at 0x%x (%s), value=0x%x\n", mem, regName, value );
	psHu32(mem) = value;
}

/////////////////////////////////////////////////////////////////////////
// HW Write 64 bit

void __fastcall hwWrite64_page_02( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval );  return;
	ipuWrite64( mem, *srcval );
}

void __fastcall hwWrite64_page_03( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;
	const u64 value = *srcval;

	if(mem>=0x10003800)
	{
		if(mem<0x10003c00)
			vif0Write32(mem, value); 
		else
			vif1Write32(mem, value); 
		return;
	}

	switch (mem)
	{
		case GIF_CTRL:
			DevCon::Status("GIF_CTRL write 64", params value);
			psHu32(mem) = value & 0x8;
			if(value & 0x1)
				gsGIFReset();
			else
			{
				if( value & 8 )
					psHu32(GIF_STAT) |= 8;
				else
					psHu32(GIF_STAT) &= ~8;
			}
	
			return;

		case GIF_MODE:
		{
#ifdef GSPATH3FIX
			Console::Status("GIFMODE64 %x\n", params value);
#endif
			psHu64(GIF_MODE) = value;

			// set/clear bits 0 and 2 as per the GIF_MODE value.
			const u32 bitmask = 0x1 | 0x4;
			psHu32(GIF_STAT) &= ~bitmask;
			psHu32(GIF_STAT) |= (u32)value & bitmask;
		}

		case GIF_STAT: // stat is readonly
			return;
	}
}

void __fastcall hwWrite64_page_0E( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;

	const u64 value = *srcval;

	if( mem == DMAC_CTRL )
	{
		HW_LOG("DMAC_CTRL Write 64bit %x\n", value);
	}
	else if( mem == DMAC_STAT )
	{
		HW_LOG("DMAC_STAT Write 64bit %x\n", value);

		// lower 16 bits: clear on 1
		// upper 16 bits: reverse on 1

		psHu16(0xe010) &= ~(value & 0xffff);
		psHu16(0xe012) ^= (u16)(value >> 16);

		cpuTestDMACInts();
		return;
	}

	psHu64(mem) = value;
}

void __fastcall hwWrite64_generic( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;

	const u64 value = *srcval;

	switch (mem)
	{
		case 0x1000a000: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite64: GSdma %x\n", cpuRegs.cycle, value);
			DmaExec(dmaGIF, mem, value);
		break;

		case INTC_STAT:
			HW_LOG("INTC_STAT Write 64bit %x\n", (u32)value);
			psHu32(INTC_STAT) &= ~value;	
			//cpuTestINTCInts();
		break;

		case INTC_MASK:
			HW_LOG("INTC_MASK Write 64bit %x\n", (u32)value);
			psHu32(INTC_MASK) ^= (u16)value;
			cpuTestINTCInts();
		break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		case 0x1000f590: // DMAC_ENABLEW
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
		break;

		default:
			psHu64(mem) = value;
			HW_LOG("Unknown Hardware write 64 at %x with value %x (status=%x)\n",mem,value, cpuRegs.CP0.n.Status.val);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////
// HW Write 128 bit

void __fastcall hwWrite128_generic(u32 mem, const mem128_t *srcval)
{
	//hwWrite128( mem, srcval ); return;

	switch (mem)
	{
		case INTC_STAT:
			HW_LOG("INTC_STAT Write 64bit %x\n", (u32)srcval[0]);
			psHu32(INTC_STAT) &= ~srcval[0];	
			//cpuTestINTCInts();
		break;
		
		case INTC_MASK:
			HW_LOG("INTC_MASK Write 64bit %x\n", (u32)srcval[0]);
			psHu32(INTC_MASK) ^= (u16)srcval[0];
			cpuTestINTCInts();
		break;

		case 0x1000f590: // DMAC_ENABLEW
			psHu32(0xf590) = srcval[0];
			psHu32(0xf520) = srcval[0];
		break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		default:
			psHu64(mem  ) = srcval[0];
			psHu64(mem+8) = srcval[1];

			HW_LOG("Unknown Hardware write 128 at %x with value %x_%x (status=%x)\n", mem, srcval[1], srcval[0], cpuRegs.CP0.n.Status.val);
		break;
	}
}
