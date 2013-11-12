/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "spu2.h"
#include <float.h>

extern void	spdif_update();

void ADMAOutLogWrite(void *lpData, u32 ulSize);

u32 core, voice;

static const s32 tbl_XA_Factor[5][2] =
{
	{    0,   0 },
	{   60,   0 },
	{  115, -52 },
	{   98, -55 },
	{  122, -60 }
};


// Performs a 64-bit multiplication between two values and returns the
// high 32 bits as a result (discarding the fractional 32 bits).
// The combined fractional bits of both inputs must be 32 bits for this
// to work properly.
//
// This is meant to be a drop-in replacement for times when the 'div' part
// of a MulDiv is a constant.  (example: 1<<8, or 4096, etc)
//
// [Air] Performance breakdown: This is over 10 times faster than MulDiv in
//   a *worst case* scenario.  It's also more accurate since it forces the
//   caller to  extend the inputs so that they make use of all 32 bits of
//   precision.
//
__forceinline s32 MulShr32( s32 srcval, s32 mulval )
{
	s64 tmp = ((s64)srcval * mulval );
	return ((s32*)&tmp)[1];

	// Performance note: Using the temp var and memory reference
	// actually ends up being roughly 2x faster than using a bitshift.
	// It won't fly on big endian machines though... :)
}

__forceinline s32 clamp_mix( s32 x, u8 bitshift )
{
	return GetClamped( x, -0x8000<<bitshift, 0x7fff<<bitshift );
}

__forceinline void clamp_mix( StereoOut32& sample, u8 bitshift )
{
	Clampify( sample.Left, -0x8000<<bitshift, 0x7fff<<bitshift );
	Clampify( sample.Right, -0x8000<<bitshift, 0x7fff<<bitshift );
}

static void __forceinline XA_decode_block(s16* buffer, const s16* block, s32& prev1, s32& prev2)
{
	const s32 header = *block;
	s32 shift =  ((header>> 0)&0xF)+16;
	s32 pred1 = tbl_XA_Factor[(header>> 4)&0xF][0];
	s32 pred2 = tbl_XA_Factor[(header>> 4)&0xF][1];

	const s8* blockbytes = (s8*)&block[1];

	for(int i=0; i<14; i++, blockbytes++)
	{
		s32 pcm, pcm2;
		{
			s32 data = ((*blockbytes)<<28) & 0xF0000000;
			pcm = data>>shift;
			pcm+=((pred1*prev1)+(pred2*prev2))>>6;
			if(pcm> 32767) pcm= 32767;
			else if(pcm<-32768) pcm=-32768;
			*(buffer++) = pcm;
		}

		//prev2=prev1;
		//prev1=pcm;

		{
			s32 data = ((*blockbytes)<<24) & 0xF0000000;
			pcm2 = data>>shift;
			pcm2+=((pred1*pcm)+(pred2*prev1))>>6;
			if(pcm2> 32767) pcm2= 32767;
			else if(pcm2<-32768) pcm2=-32768;
			*(buffer++) = pcm2;
		}

		prev2=pcm;
		prev1=pcm2;
	}
}

static void __forceinline XA_decode_block_unsaturated(s16* buffer, const s16* block, s32& prev1, s32& prev2)
{
	const s32 header = *block;
	s32 shift =  ((header>> 0)&0xF)+16;
	s32 pred1 = tbl_XA_Factor[(header>> 4)&0xF][0];
	s32 pred2 = tbl_XA_Factor[(header>> 4)&0xF][1];

	const s8* blockbytes = (s8*)&block[1];

	for(int i=0; i<14; i++, blockbytes++)
	{
		s32 pcm, pcm2;
		{
			s32 data = ((*blockbytes)<<28) & 0xF0000000;
			pcm = data>>shift;
			pcm+=((pred1*prev1)+(pred2*prev2))>>6;
			// [Air] : Fast method, no saturation is performed.
			*(buffer++) = pcm;
		}

		{
			s32 data = ((*blockbytes)<<24) & 0xF0000000;
			pcm2 = data>>shift;
			pcm2+=((pred1*pcm)+(pred2*prev1))>>6;
			// [Air] : Fast method, no saturation is performed.
			*(buffer++) = pcm2;
		}

		prev2 = pcm;
		prev1 = pcm2;
	}
}

static void __forceinline IncrementNextA( const V_Core& thiscore, V_Voice& vc )
{
	// Important!  Both cores signal IRQ when an address is read, regardless of
	// which core actually reads the address.

	for( int i=0; i<2; i++ )
	{
		if( Cores[i].IRQEnable && (vc.NextA==Cores[i].IRQA ) )
		{ 
			if( IsDevBuild )
				ConLog(" * SPU2 Core %d: IRQ Called (IRQ passed).\n", i); 

			Spdif.Info = 4 << i;
			SetIrqCall();
		}
	}

	vc.NextA++;
	vc.NextA&=0xFFFFF;
}

// decoded pcm data, used to cache the decoded data so that it needn't be decoded
// multiple times.  Cache chunks are decoded when the mixer requests the blocks, and
// invalided when DMA transfers and memory writes are performed.
PcmCacheEntry *pcm_cache_data = NULL;

int g_counter_cache_hits = 0;
int g_counter_cache_misses = 0;
int g_counter_cache_ignores = 0;

#define XAFLAG_LOOP_END		(1ul<<0)
#define XAFLAG_LOOP			(1ul<<1)
#define XAFLAG_LOOP_START	(1ul<<2)

static s32 __forceinline __fastcall GetNextDataBuffered( V_Core& thiscore, V_Voice& vc ) 
{
	if (vc.SCurrent<28)
	{
		// [Air] : skip the increment?
		//    (witness one of the rare ideal uses of a goto statement!)
		if( (vc.SCurrent&3) != 3 ) goto _skipIncrement;
	}
	else
	{
		if(vc.LoopFlags & XAFLAG_LOOP_END)
		{
			thiscore.Regs.ENDX |= (1 << voice);

			if( vc.LoopFlags & XAFLAG_LOOP )
			{
				vc.NextA=vc.LoopStartA;
			}
			else
			{
				vc.Stop();
				if( IsDevBuild )
				{
					if(MsgVoiceOff()) ConLog(" * SPU2: Voice Off by EndPoint: %d \n", voice);
					DebugCores[core].Voices[voice].lastStopReason = 1;
				}
			}
		}

		// We'll need the loop flags and buffer pointers regardless of cache status:
		// Note to Self : NextA addresses WORDS (not bytes).

		s16* memptr = GetMemPtr(vc.NextA&0xFFFFF);
		vc.LoopFlags = *memptr >> 8;	// grab loop flags from the upper byte.

		const int cacheIdx = vc.NextA / pcm_WordsPerBlock;
		PcmCacheEntry& cacheLine = pcm_cache_data[cacheIdx];
		vc.SBuffer = cacheLine.Sampledata;

		if( cacheLine.Validated )
		{
			// Cached block!  Read from the cache directly.
			// Make sure to propagate the prev1/prev2 ADPCM:

			vc.Prev1 = vc.SBuffer[27];
			vc.Prev2 = vc.SBuffer[26];

			//ConLog( " * SPU2 : Cache Hit! NextA=0x%x, cacheIdx=0x%x\n", vc.NextA, cacheIdx );

			if( IsDevBuild )
				g_counter_cache_hits++;
		}
		else
		{
			// Only flag the cache if it's a non-dynamic memory range.
			if( vc.NextA >= SPU2_DYN_MEMLINE )
				cacheLine.Validated = true;

			if( IsDevBuild )
			{
				if( vc.NextA < SPU2_DYN_MEMLINE )
					g_counter_cache_ignores++;
				else
					g_counter_cache_misses++;
			}

			s16* sbuffer = cacheLine.Sampledata;

			// saturated decoder
			//XA_decode_block( sbuffer, memptr, vc.Prev1, vc.Prev2 );

			// [Air]: Testing use of a new unsaturated decoder. (benchmark needed)
			//   Chances are the saturation isn't needed, but for a very few exception games.
			//   This is definitely faster than the above version, but is it by enough to
			//   merit possible lower compatibility?  Especially now that games that make
			//   heavy use of the SPU2 via music or sfx will mostly use the cache anyway.

			XA_decode_block_unsaturated( vc.SBuffer, memptr, vc.Prev1, vc.Prev2 );
		}

		vc.SCurrent = 0;
		if( (vc.LoopFlags & XAFLAG_LOOP_START) && !vc.LoopMode )
			vc.LoopStartA = vc.NextA;
	}

	IncrementNextA( thiscore, vc );

_skipIncrement:
	return vc.SBuffer[vc.SCurrent++];
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
static s32 __forceinline GetNoiseValues()
{
	static s32 Seed = 0x41595321;
	s32 retval = 0x8000;

	if( Seed&0x100 ) retval = (Seed&0xff) << 8;
	else if( Seed&0xffff ) retval = 0x7fff;

	__asm {
		MOV eax,Seed
		ROR eax,5
		XOR eax,0x9a
		MOV ebx,eax
		ROL eax,2
		ADD eax,ebx
		XOR eax,ebx
		ROR eax,3
		MOV Seed,eax
	}
	return retval;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

// Data is expected to be 16 bit signed (typical stuff!).
// volume is expected to be 32 bit signed (31 bits with reverse phase)
// Output is effectively 15 bit range, thanks to the signed volume.
static __forceinline s32 ApplyVolume(s32 data, s32 volume)
{
	//return (volume * data) >> 15;
	return MulShr32( data<<1, volume );
}

static __forceinline StereoOut32 ApplyVolume( const StereoOut32& data, const V_VolumeLR& volume )
{
	return StereoOut32(
		ApplyVolume( data.Left, volume.Left ),
		ApplyVolume( data.Right, volume.Right )
	);
}

static __forceinline StereoOut32 ApplyVolume( const StereoOut32& data, const V_VolumeSlideLR& volume )
{
	return StereoOut32(
		ApplyVolume( data.Left, volume.Left.Value ),
		ApplyVolume( data.Right, volume.Right.Value )
	);
}

static void __forceinline UpdatePitch( V_Voice& vc )
{
	s32 pitch;

	// [Air] : re-ordered comparisons: Modulated is much more likely to be zero than voice,
	//   and so the way it was before it's have to check both voice and modulated values
	//   most of the time.  Now it'll just check Modulated and short-circuit past the voice
	//   check (not that it amounts to much, but eh every little bit helps).
	if( (vc.Modulated==0) || (voice==0) )
		pitch = vc.Pitch;
	else
		pitch = (vc.Pitch*(32768 + abs(Cores[core].Voices[voice-1].OutX)))>>15;
	
	vc.SP+=pitch;
}


static __forceinline void CalculateADSR( V_Core& thiscore, V_Voice& vc )
{
	if( vc.ADSR.Phase==0 )
	{
		vc.ADSR.Value = 0;
		return;
	}

	if( !vc.ADSR.Calculate() )
	{
		if( IsDevBuild )
		{
			if(MsgVoiceOff()) ConLog(" * SPU2: Voice Off by ADSR: %d \n", voice);
			DebugCores[core].Voices[voice].lastStopReason = 2;
		}
		vc.Stop();
		thiscore.Regs.ENDX |= (1 << voice);
	}

	jASSUME( vc.ADSR.Value >= 0 );	// ADSR should never be negative...
}

// Returns a 16 bit result in Value.
static s32 __forceinline GetVoiceValues_Linear( V_Core& thiscore, V_Voice& vc )
{
	while( vc.SP > 0 )
	{
		vc.PV2 = vc.PV1;
		vc.PV1 = GetNextDataBuffered( thiscore, vc );
		vc.SP -= 4096;
	}

	CalculateADSR( thiscore, vc );

	// Note!  It's very important that ADSR stay as accurate as possible.  By the way
	// it is used, various sound effects can end prematurely if we truncate more than
	// one or two bits.

	if(Interpolation==0)
	{
		return ApplyVolume( vc.PV1, vc.ADSR.Value );
	} 
	else //if(Interpolation==1) //must be linear
	{
		s32 t0 = vc.PV2 - vc.PV1;
		return MulShr32( (vc.PV1<<1) - ((t0*vc.SP)>>11), vc.ADSR.Value );
	}
}

// Returns a 16 bit result in Value.
static s32 __forceinline GetVoiceValues_Cubic( V_Core& thiscore, V_Voice& vc )
{
	while( vc.SP > 0 )
	{
		vc.PV4 = vc.PV3;
		vc.PV3 = vc.PV2;
		vc.PV2 = vc.PV1;

		vc.PV1 = GetNextDataBuffered( thiscore, vc );
		vc.PV1 <<= 2;
		vc.SPc = vc.SP&4095;	// just the fractional part, please!
		vc.SP -= 4096;
	}

	CalculateADSR( thiscore, vc );

	s32 z0 = vc.PV3 - vc.PV4 + vc.PV1 - vc.PV2;
	s32 z1 = (vc.PV4 - vc.PV3 - z0);
	s32 z2 = (vc.PV2 - vc.PV4);

	s32 mu = vc.SPc;

	s32 val = (z0 * mu) >> 12;
	val = ((val + z1) * mu) >> 12;
	val = ((val + z2) * mu) >> 12;
	val += vc.PV2;

	// Note!  It's very important that ADSR stay as accurate as possible.  By the way
	// it is used, various sound effects can end prematurely if we truncate more than
	// one or two bits.
	return MulShr32( val, vc.ADSR.Value>>1 );
}

// Noise values need to be mixed without going through interpolation, since it
// can wreak havoc on the noise (causing muffling or popping).  Not that this noise
// generator is accurate in its own right.. but eh, ah well :)
static s32 __forceinline __fastcall GetNoiseValues( V_Core& thiscore, V_Voice& vc )
{
	s32 retval = GetNoiseValues();

	/*while(vc.SP>=4096)
	{
		retval = GetNoiseValues();
		vc.SP-=4096;
	}*/

	// GetNoiseValues can't set the phase zero on us unexpectedly
	// like GetVoiceValues can.  Better assert just in case though..
	jASSUME( vc.ADSR.Phase != 0 );

	CalculateADSR( thiscore, vc );

	// Yup, ADSR applies even to noise sources...
	return ApplyVolume( retval, vc.ADSR.Value );
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

void __fastcall ReadInput( V_Core& thiscore, StereoOut32& PData ) 
{
	if((thiscore.AutoDMACtrl&(core+1))==(core+1))
	{
		s32 tl,tr;

		if((core==1)&&((PlayMode&8)==8))
		{
			thiscore.InputPos&=~1;

			// CDDA mode
			// Source audio data is 32 bits.
			// We don't yet have the capability to handle this high res input data
			// so we just downgrade it to 16 bits for now.
			
#ifdef PCM24_S1_INTERLEAVE
			*PData.Left=*(((s32*)(thiscore.ADMATempBuffer+(thiscore.InputPos<<1))));
			*PData.Right=*(((s32*)(thiscore.ADMATempBuffer+(thiscore.InputPos<<1)+2)));
#else
			s32 *pl=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos]);
			s32 *pr=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos+0x200]);
			PData.Left = *pl;
			PData.Right = *pr;
#endif

			PData.Left >>= 2; //give 30 bit data (SndOut downsamples the rest of the way)
			PData.Right >>= 2;

			thiscore.InputPos+=2;
			if((thiscore.InputPos==0x100)||(thiscore.InputPos>=0x200)) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

#ifdef PCM24_S1_INTERLEAVE
					AutoDMAReadBuffer(core,1);
#else
					AutoDMAReadBuffer(core,0);
#endif
					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						FileLog("[%10d] AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');

						if( IsDevBuild )
						{
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}
						thiscore.InputDataLeft=0;
						thiscore.DMAICounter=1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}

		}
		else if((core==0)&&((PlayMode&4)==4))
		{
			thiscore.InputPos&=~1;

			s32 *pl=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos]);
			s32 *pr=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos+0x200]);
			PData.Left  = *pl;
			PData.Right = *pr;

			thiscore.InputPos+=2;
			if(thiscore.InputPos>=0x200) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

					AutoDMAReadBuffer(core,0);

					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						FileLog("[%10d] Spdif AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');

						if( IsDevBuild )
						{
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}
						thiscore.InputDataLeft=0;
						thiscore.DMAICounter=1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}

		}
		else
		{
			if((core==1)&&((PlayMode&2)!=0))
			{
				tl=0;
				tr=0;
			}
			else
			{
				// Using the temporary buffer because this area gets overwritten by some other code.
				//*PData.Left  = (s32)*(s16*)(spu2mem+0x2000+(core<<10)+thiscore.InputPos);
				//*PData.Right = (s32)*(s16*)(spu2mem+0x2200+(core<<10)+thiscore.InputPos);

				tl = (s32)thiscore.ADMATempBuffer[thiscore.InputPos];
				tr = (s32)thiscore.ADMATempBuffer[thiscore.InputPos+0x200];

			}

			PData.Left  = tl;
			PData.Right = tr;

			thiscore.InputPos++;
			if((thiscore.InputPos==0x100)||(thiscore.InputPos>=0x200)) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

					AutoDMAReadBuffer(core,0);

					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						thiscore.AutoDMACtrl |= ~3;

						if( IsDevBuild )
						{
							FileLog("[%10d] AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}

						thiscore.InputDataLeft = 0;
						thiscore.DMAICounter   = 1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}
		}
	}
	else
	{
		PData.Left  = 0;
		PData.Right = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

static __forceinline StereoOut32 ReadInputPV( V_Core& thiscore ) 
{
	u32 pitch=AutoDMAPlayRate[core];

	if(pitch==0) pitch=48000;
	
	thiscore.ADMAPV += pitch;
	while(thiscore.ADMAPV>=48000) 
	{
		ReadInput( thiscore, thiscore.ADMAP );
		thiscore.ADMAPV -= 48000;
	}

	// Apply volumes:
	return ApplyVolume( thiscore.ADMAP, thiscore.InpVol );
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

// writes a signed value to the SPU2 ram
// Performs no cache invalidation -- use only for dynamic memory ranges
// of the SPU2 (between 0x0000 and SPU2_DYN_MEMLINE)
static __forceinline void spu2M_WriteFast( u32 addr, s16 value )
{
	// throw an assertion if the memory range is invalid:
#ifndef _DEBUG_FAST
	jASSUME( addr < SPU2_DYN_MEMLINE );
#endif
	*GetMemPtr( addr ) = value;
}


static __forceinline StereoOut32 MixVoice( V_Core& thiscore, V_Voice& vc )
{
	// Most games don't use much volume slide effects.  So only call the UpdateVolume
	// methods when needed by checking the flag outside the method here...

	vc.Volume.Update();

	// SPU2 Note: The spu2 continues to process voices for eternity, always, so we
	// have to run through all the motions of updating the voice regardless of it's
	// audible status.  Otherwise IRQs might not trigger and emulation might fail.
	
	if( vc.ADSR.Phase > 0 )
	{
		UpdatePitch( vc );

		s32 Value;

		if( vc.Noise )
			Value = GetNoiseValues( thiscore, vc );
		else
		{
			if( Interpolation == 2 )
				Value = GetVoiceValues_Cubic( thiscore, vc );
			else
				Value = GetVoiceValues_Linear( thiscore, vc );
		}

		// Note: All values recorded into OutX (may be used for modulation later)
		vc.OutX = Value;

		if( IsDevBuild )
			DebugCores[core].Voices[voice].displayPeak = max(DebugCores[core].Voices[voice].displayPeak,abs(vc.OutX));

		// Write-back of raw voice data (post ADSR applied)

		if (voice==1)      spu2M_WriteFast( 0x400 + (core<<12) + OutPos, Value );
		else if (voice==3) spu2M_WriteFast( 0x600 + (core<<12) + OutPos, Value );

		return ApplyVolume( StereoOut32( Value, Value ), vc.Volume );
	}
	else
	{
		// Write-back of raw voice data (some zeros since the voice is "dead")

		if (voice==1)      spu2M_WriteFast( 0x400 + (core<<12) + OutPos, 0 );
		else if (voice==3) spu2M_WriteFast( 0x600 + (core<<12) + OutPos, 0 );
		
		return StereoOut32( 0, 0 );
	}
}


static StereoOut32 __fastcall MixCore( const StereoOut32& Input, const StereoOut32& Ext )
{
	V_Core& thiscore( Cores[core] );
	thiscore.MasterVol.Update();

	StereoOut32 Dry(0,0), Wet(0,0);

	for( voice=0; voice<24; ++voice )
	{
		V_Voice& vc( thiscore.Voices[voice] );
		StereoOut32 VVal( MixVoice( thiscore, vc ) );
		
		// Note: Results from MixVoice are ranged at 16 bits.

		Dry.Left += VVal.Left & vc.DryL;
		Dry.Right += VVal.Right & vc.DryR;
		Wet.Left += VVal.Left & vc.WetL;
		Wet.Right += VVal.Right & vc.WetR;
	}
	
	// Saturate final result to standard 16 bit range.
	clamp_mix( Dry );
	clamp_mix( Wet );
	
	// Write Mixed results To Output Area
	spu2M_WriteFast( 0x1000 + (core<<12) + OutPos, Dry.Left );
	spu2M_WriteFast( 0x1200 + (core<<12) + OutPos, Dry.Right );
	spu2M_WriteFast( 0x1400 + (core<<12) + OutPos, Wet.Left );
	spu2M_WriteFast( 0x1600 + (core<<12) + OutPos, Wet.Right );
	
	// Write mixed results to logfile (if enabled)
	
	WaveDump::WriteCore( core, CoreSrc_DryVoiceMix, Dry );
	WaveDump::WriteCore( core, CoreSrc_WetVoiceMix, Wet );

	// Mix in the Input data

	StereoOut32 TD(
		Input.Left & thiscore.InpDryL,
		Input.Right & thiscore.InpDryR
	);
	
	// Mix in the Voice data
	TD.Left += Dry.Left & thiscore.SndDryL;
	TD.Right += Dry.Right & thiscore.SndDryR;

	// Mix in the External (nothing/core0) data
	TD.Left += Ext.Left & thiscore.ExtDryL;
	TD.Right += Ext.Right & thiscore.ExtDryR;
	
	if( !EffectsDisabled )
	{
		//Reverb pointer advances regardless of the FxEnable bit...
		Reverb_AdvanceBuffer( thiscore );

		if( thiscore.FxEnable )
		{
			// Mix Input, Voice, and External data:
			StereoOut32 TW(
				Input.Left & thiscore.InpWetL,
				Input.Right & thiscore.InpWetR
			);
			
			TW.Left += Wet.Left & thiscore.SndWetL;
			TW.Right += Wet.Right & thiscore.SndWetR;
			TW.Left += Ext.Left & thiscore.ExtWetL; 
			TW.Right += Ext.Right & thiscore.ExtWetR;

			WaveDump::WriteCore( core, CoreSrc_PreReverb, TW );

			StereoOut32 RV( DoReverb( thiscore, TW ) );

			// Volume boost after effects application.  Boosting volume prior to effects
			// causes slight overflows in some games, and the volume boost is required.
			// (like all over volumes on SPU2, reverb coefficients and stuff are signed,
			// range -50% to 50%, thus *2 is needed)
			
			RV.Left  *= 2;
			RV.Right *= 2;

			WaveDump::WriteCore( core, CoreSrc_PostReverb, RV );

			// Mix Dry+Wet
			return StereoOut32( TD + ApplyVolume( RV, thiscore.FxVol ) );
		}
		else
		{
			WaveDump::WriteCore( core, CoreSrc_PreReverb, 0, 0 );
			WaveDump::WriteCore( core, CoreSrc_PostReverb, 0, 0 );
		}
	}
	return TD;
}

// used to throttle the output rate of cache stat reports
static int p_cachestat_counter=0;

__forceinline void Mix() 
{
	// ****  CORE ZERO  ****
	core = 0;

	// Note: Playmode 4 is SPDIF, which overrides other inputs.
	StereoOut32 Ext( (PlayMode&4) ? StereoOut32::Empty : ReadInputPV( Cores[0] ) );
	WaveDump::WriteCore( 0, CoreSrc_Input, Ext );

	Ext = MixCore( Ext, StereoOut32::Empty );

	if( (PlayMode & 4) || (Cores[0].Mute!=0) )
		Ext = StereoOut32( 0, 0 );
	else
	{
		Ext = ApplyVolume( Ext, Cores[0].MasterVol );
		clamp_mix( Ext );
	}

	// Commit Core 0 output to ram before mixing Core 1:

	spu2M_WriteFast( 0x800 + OutPos, Ext.Left );
	spu2M_WriteFast( 0xA00 + OutPos, Ext.Right );
	WaveDump::WriteCore( 0, CoreSrc_External, Ext );

	// ****  CORE ONE  ****

	core = 1;
	StereoOut32 Out( (PlayMode&8) ? StereoOut32::Empty : ReadInputPV( Cores[1] ) );
	WaveDump::WriteCore( 1, CoreSrc_Input, Out );

	ApplyVolume( Ext, Cores[1].ExtVol );
	Out = MixCore( Out, Ext );

	if( PlayMode & 8 )
	{
		// Experimental CDDA support
		// The CDDA overrides all other mixer output.  It's a direct feed!

		ReadInput( Cores[1], Out );
		//WaveLog::WriteCore( 1, "CDDA-32", OutL, OutR );
	}
	else
	{
		Out.Left = MulShr32( Out.Left<<SndOutVolumeShift, Cores[1].MasterVol.Left.Value );
		Out.Right = MulShr32( Out.Right<<SndOutVolumeShift, Cores[1].MasterVol.Right.Value );

		// Final Clamp!
		// This could be circumvented by using 1/2th total output volume, although
		// I suspect this approach (clamping at the higher volume) is more true to the
		// PS2's real implementation.

		clamp_mix( Out, SndOutVolumeShift );
	}

	// Update spdif (called each sample)
	if(PlayMode&4)
		spdif_update();

	SndBuffer::Write( Out );
	
	// Update AutoDMA output positioning
	OutPos++;
	if (OutPos>=0x200) OutPos=0;

	if( IsDevBuild )
	{
		p_cachestat_counter++;
		if(p_cachestat_counter > (48000*10) )
		{
			p_cachestat_counter = 0;
			if( MsgCache() ) ConLog( " * SPU2 > CacheStats > Hits: %d  Misses: %d  Ignores: %d\n",
				g_counter_cache_hits,
				g_counter_cache_misses,
				g_counter_cache_ignores );

			g_counter_cache_hits = 
			g_counter_cache_misses =
			g_counter_cache_ignores = 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

/*
-----------------------------------------------------------------------------
PSX reverb hardware notes
by Neill Corlett
-----------------------------------------------------------------------------

Yadda yadda disclaimer yadda probably not perfect yadda well it's okay anyway
yadda yadda.

-----------------------------------------------------------------------------

Basics
------

- The reverb buffer is 22khz 16-bit mono PCM.
- It starts at the reverb address given by 1DA2, extends to
  the end of sound RAM, and wraps back to the 1DA2 address.

Setting the address at 1DA2 resets the current reverb work address.

This work address ALWAYS increments every 1/22050 sec., regardless of
whether reverb is enabled (bit 7 of 1DAA set).

And the contents of the reverb buffer ALWAYS play, scaled by the
"reverberation depth left/right" volumes (1D84/1D86).
(which, by the way, appear to be scaled so 3FFF=approx. 1.0, 4000=-1.0)

-----------------------------------------------------------------------------

Register names
--------------

These are probably not their real names.
These are probably not even correct names.
We will use them anyway, because we can.

1DC0: FB_SRC_A       (offset)
1DC2: FB_SRC_B       (offset)
1DC4: IIR_ALPHA      (coef.)
1DC6: ACC_COEF_A     (coef.)
1DC8: ACC_COEF_B     (coef.)
1DCA: ACC_COEF_C     (coef.)
1DCC: ACC_COEF_D     (coef.)
1DCE: IIR_COEF       (coef.)
1DD0: FB_ALPHA       (coef.)
1DD2: FB_X           (coef.)
1DD4: IIR_DEST_A0    (offset)
1DD6: IIR_DEST_A1    (offset)
1DD8: ACC_SRC_A0     (offset)
1DDA: ACC_SRC_A1     (offset)
1DDC: ACC_SRC_B0     (offset)
1DDE: ACC_SRC_B1     (offset)
1DE0: IIR_SRC_A0     (offset)
1DE2: IIR_SRC_A1     (offset)
1DE4: IIR_DEST_B0    (offset)
1DE6: IIR_DEST_B1    (offset)
1DE8: ACC_SRC_C0     (offset)
1DEA: ACC_SRC_C1     (offset)
1DEC: ACC_SRC_D0     (offset)
1DEE: ACC_SRC_D1     (offset)
1DF0: IIR_SRC_B1     (offset)
1DF2: IIR_SRC_B0     (offset)
1DF4: MIX_DEST_A0    (offset)
1DF6: MIX_DEST_A1    (offset)
1DF8: MIX_DEST_B0    (offset)
1DFA: MIX_DEST_B1    (offset)
1DFC: IN_COEF_L      (coef.)
1DFE: IN_COEF_R      (coef.)

The coefficients are signed fractional values.
-32768 would be -1.0
 32768 would be  1.0 (if it were possible... the highest is of course 32767)

The offsets are (byte/8) offsets into the reverb buffer.
i.e. you multiply them by 8, you get byte offsets.
You can also think of them as (samples/4) offsets.
They appear to be signed.  They can be negative.
None of the documented presets make them negative, though.

Yes, 1DF0 and 1DF2 appear to be backwards.  Not a typo.

-----------------------------------------------------------------------------

What it does
------------

We take all reverb sources:
- regular channels that have the reverb bit on
- cd and external sources, if their reverb bits are on
and mix them into one stereo 44100hz signal.

Lowpass/downsample that to 22050hz.  The PSX uses a proper bandlimiting
algorithm here, but I haven't figured out the hysterically exact specifics.
I use an 8-tap filter with these coefficients, which are nice but probably
not the real ones:

0.037828187894
0.157538631280
0.321159685278
0.449322115345
0.449322115345
0.321159685278
0.157538631280
0.037828187894

So we have two input samples (INPUT_SAMPLE_L, INPUT_SAMPLE_R) every 22050hz.

* IN MY EMULATION, I divide these by 2 to make it clip less.
  (and of course the L/R output coefficients are adjusted to compensate)
  The real thing appears to not do this.

At every 22050hz tick:
- If the reverb bit is enabled (bit 7 of 1DAA), execute the reverb
  steady-state algorithm described below
- AFTERWARDS, retrieve the "wet out" L and R samples from the reverb buffer
  (This part may not be exactly right and I guessed at the coefs. TODO: check later.)
  L is: 0.333 * (buffer[MIX_DEST_A0] + buffer[MIX_DEST_B0])
  R is: 0.333 * (buffer[MIX_DEST_A1] + buffer[MIX_DEST_B1])
- Advance the current buffer position by 1 sample

The wet out L and R are then upsampled to 44100hz and played at the
"reverberation depth left/right" (1D84/1D86) volume, independent of the main
volume.

-----------------------------------------------------------------------------

Reverb steady-state
-------------------

The reverb steady-state algorithm is fairly clever, and of course by
"clever" I mean "batshit insane".

buffer[x] is relative to the current buffer position, not the beginning of
the buffer.  Note that all buffer offsets must wrap around so they're
contained within the reverb work area.

Clipping is performed at the end... maybe also sooner, but definitely at
the end.

IIR_INPUT_A0 = buffer[IIR_SRC_A0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
IIR_INPUT_A1 = buffer[IIR_SRC_A1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;
IIR_INPUT_B0 = buffer[IIR_SRC_B0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
IIR_INPUT_B1 = buffer[IIR_SRC_B1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;

IIR_A0 = IIR_INPUT_A0 * IIR_ALPHA + buffer[IIR_DEST_A0] * (1.0 - IIR_ALPHA);
IIR_A1 = IIR_INPUT_A1 * IIR_ALPHA + buffer[IIR_DEST_A1] * (1.0 - IIR_ALPHA);
IIR_B0 = IIR_INPUT_B0 * IIR_ALPHA + buffer[IIR_DEST_B0] * (1.0 - IIR_ALPHA);
IIR_B1 = IIR_INPUT_B1 * IIR_ALPHA + buffer[IIR_DEST_B1] * (1.0 - IIR_ALPHA);

buffer[IIR_DEST_A0 + 1sample] = IIR_A0;
buffer[IIR_DEST_A1 + 1sample] = IIR_A1;
buffer[IIR_DEST_B0 + 1sample] = IIR_B0;
buffer[IIR_DEST_B1 + 1sample] = IIR_B1;

ACC0 = buffer[ACC_SRC_A0] * ACC_COEF_A +
       buffer[ACC_SRC_B0] * ACC_COEF_B +
       buffer[ACC_SRC_C0] * ACC_COEF_C +
       buffer[ACC_SRC_D0] * ACC_COEF_D;
ACC1 = buffer[ACC_SRC_A1] * ACC_COEF_A +
       buffer[ACC_SRC_B1] * ACC_COEF_B +
       buffer[ACC_SRC_C1] * ACC_COEF_C +
       buffer[ACC_SRC_D1] * ACC_COEF_D;

FB_A0 = buffer[MIX_DEST_A0 - FB_SRC_A];
FB_A1 = buffer[MIX_DEST_A1 - FB_SRC_A];
FB_B0 = buffer[MIX_DEST_B0 - FB_SRC_B];
FB_B1 = buffer[MIX_DEST_B1 - FB_SRC_B];

buffer[MIX_DEST_A0] = ACC0 - FB_A0 * FB_ALPHA;
buffer[MIX_DEST_A1] = ACC1 - FB_A1 * FB_ALPHA;
buffer[MIX_DEST_B0] = (FB_ALPHA * ACC0) - FB_A0 * (FB_ALPHA^0x8000) - FB_B0 * FB_X;
buffer[MIX_DEST_B1] = (FB_ALPHA * ACC1) - FB_A1 * (FB_ALPHA^0x8000) - FB_B1 * FB_X;

-----------------------------------------------------------------------------
*/
