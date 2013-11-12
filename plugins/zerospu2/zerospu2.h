/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SPU2_H__
#define __SPU2_H__

#include <stdio.h>
#include <string.h>
#include <malloc.h>

extern "C" {
#define SPU2defs
#include "PS2Edefs.h"
}

#include "reg.h"
#include "misc.h"

#include <string>
#include <vector>
using namespace std;

extern FILE *spu2Log;

// Prints most of the function names of the callbacks as they are called by pcsx2. 
// I'm keeping the code in because I have a feeling it will come in handy.
//#define PRINT_CALLBACKS

#ifdef PRINT_CALLBACKS
#define LOG_CALLBACK printf
#else
#define LOG_CALLBACK 0&&
#endif

#ifdef _DEBUG
#define SPU2_LOG __Log  //debug mode
#else
#define SPU2_LOG 0&&
#endif

#define  SPU2_VERSION  PS2E_SPU2_VERSION
#define SPU2_REVISION 0
#define  SPU2_BUILD	4	// increase that with each version
#define SPU2_MINOR 6

#define OPTION_TIMESTRETCH 1 // stretches samples without changing pitch to reduce cracking
#define OPTION_REALTIME 2 // sync to real time instead of ps2 time
#define OPTION_MUTE 4   // don't output anything
#define OPTION_RECORDING 8

// ADSR constants
#define ATTACK_MS	  494L
#define DECAYHALF_MS   286L
#define DECAY_MS	   572L
#define SUSTAIN_MS	 441L
#define RELEASE_MS	 437L

#define CYCLES_PER_MS (36864000/1000)

#define AUDIO_BUFFER 2048

#define NSSIZE	  48	  // ~ 1 ms of data
#define NSFRAMES	16	  // gather at least NSFRAMES of NSSIZE before submitting
#define NSPACKETS 24

#define SPU_NUMBER_VOICES	   48

#define SAMPLE_RATE 48000L
#define RECORD_FILENAME "zerospu2.wav"

extern s8 *spu2regs;
extern u16* spu2mem;
extern int iFMod[NSSIZE];
extern u32 MemAddr[2];
extern unsigned long   dwNoiseVal;						  // global noise generator

// functions of main emu, called on spu irq
extern void (*irqCallbackSPU2)();
extern void (*irqCallbackDMA4)();
extern void (*irqCallbackDMA7)();

extern int SPUCycles, SPUWorkerCycles;
extern int SPUStartCycle[2];
extern int SPUTargetCycle[2];

extern u16 interrupt;

typedef struct {
	int Log;
	int options;
} Config;

extern Config conf;

void __Log(char *fmt, ...);
void SaveConfig();
void LoadConfig();
void SysMessage(char *fmt, ...);

void LogRawSound(void* pleft, int leftstride, void* pright, int rightstride, int numsamples);
void LogPacketSound(void* packet, int memsize);

// simulate SPU2 for 1ms
void SPU2Worker();

// hardware sound functions
int SetupSound(); // if successful, returns 0
void RemoveSound();
int SoundGetBytesBuffered();
// returns 0 is successful, else nonzero
void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#define clamp16(dest) \
{ \
			if ( dest < -32768L ) \
				dest = -32768L; \
			else if ( dest > 32767L )  \
				dest = 32767L; \
}

#define clampandwrite16(dest,value) \
{ \
			if ( value < -32768 ) \
				dest = -32768; \
			else if ( value > 32767 )  \
				dest = 32767; \
			else  \
				dest = (s16)value; \
}

#define spu2Rs16(mem)	(*(s16*)&spu2regs[(mem) & 0xffff])
#define spu2Ru16(mem)	(*(u16*)&spu2regs[(mem) & 0xffff])

#define IRQINFO spu2Ru16(REG_IRQINFO)

static __forceinline u32 SPU2_GET32BIT(u32 lo, u32 hi)
{
	return (((u32)(spu2Ru16(hi) & 0x3f) << 16) | (u32)spu2Ru16(lo));
}

static __forceinline void SPU2_SET32BIT(u32 value, u32 lo, u32 hi)
{
	spu2Ru16(hi) = ((value) >> 16) & 0x3f;
	spu2Ru16(lo) = (value) & 0xffff;
}

static __forceinline u32 C0_IRQA()
{
	return SPU2_GET32BIT(REG_C0_IRQA_LO, REG_C0_IRQA_HI);
}

static __forceinline u32 C1_IRQA()
{
	return SPU2_GET32BIT(REG_C1_IRQA_LO, REG_C1_IRQA_HI);
}

static __forceinline u32 C_IRQA(int c)
{
	if (c == 0)
		return C0_IRQA();
	else
		return C1_IRQA();
}

static __forceinline u32 C0_SPUADDR()
{
	return SPU2_GET32BIT(REG_C0_SPUADDR_LO, REG_C0_SPUADDR_HI);
}

static __forceinline u32 C1_SPUADDR()
{
	return SPU2_GET32BIT(REG_C1_SPUADDR_LO, REG_C1_SPUADDR_HI);
}

static __forceinline u32 C_SPUADDR(int c)
{
	if (c == 0)
		return C0_SPUADDR();
	else
		return C1_SPUADDR();
}

static __forceinline void C0_SPUADDR_SET(u32 value)
{
	SPU2_SET32BIT(value, REG_C0_SPUADDR_LO, REG_C0_SPUADDR_HI);
}

static __forceinline void C1_SPUADDR_SET(u32 value)
{
	SPU2_SET32BIT(value, REG_C1_SPUADDR_LO, REG_C1_SPUADDR_HI);
}

static __forceinline void C_SPUADDR_SET(u32 value, int c)
{
	if (c == 0)
		C0_SPUADDR_SET(value);
	else
		C1_SPUADDR_SET(value);
}

struct SPU_CONTROL_
{
	u16 extCd : 1;
	u16 extAudio : 1;
	u16 cdreverb : 1; 
	u16 extr : 1;	  // external reverb
	u16 dma : 2;	   // 1 - no dma, 2 - write, 3 - read
	u16 irq : 1;
	u16 reverb : 1;
	u16 noiseFreq : 6;
	u16 spuUnmute : 1;
	u16 spuon : 1;
};

#if defined(_MSC_VER)
#pragma pack(1)
#endif
// the layout of each voice in wSpuRegs
struct _SPU_VOICE
{
	union
	{
		struct {
			u16 Vol : 14;
			u16 Inverted : 1;
			u16 Sweep0 : 1;
		} vol;
		struct {
			u16 Vol : 7;
			u16 res1 : 5;
			u16 Inverted : 1;
			u16 Decrease : 1;  // if 0, increase
			u16 ExpSlope : 1;  // if 0, linear slope
			u16 Sweep1 : 1;	// always one
		} sweep;
		u16 word;
	} left, right;

	u16 pitch : 14;		// 1000 - no pitch, 2000 - pitch + 1, etc
	u16 res0 : 2;

	u16 SustainLvl : 4;
	u16 DecayRate : 4;
	u16 AttackRate : 7; 
	u16 AttackExp : 1;	 // if 0, linear
	
	u16 ReleaseRate : 5;
	u16 ReleaseExp : 1;	// if 0, linear
	u16 SustainRate : 7;
	u16 res1 : 1;
	u16 SustainDec : 1;	// if 0, inc
	u16 SustainExp : 1;	// if 0, linear
	
	u16 AdsrVol;
	u16 Address;		   // add / 8
	u16 RepeatAddr;		// gets reset when sample starts
#if defined(_MSC_VER)
};						//+22
#else
} __attribute__((packed));
#endif

// ADSR INFOS PER   CHANNEL
struct ADSRInfoEx
{
	int			State;
	int			AttackModeExp;
	int			AttackRate;
	int			DecayRate;
	int			SustainLevel;
	int			SustainModeExp;
	int			SustainIncrease;
	int			SustainRate;
	int			ReleaseModeExp;
	int			ReleaseRate;
	int			EnvelopeVol;
	long		   lVolume;
};

#define SPU_VOICE_STATE_SIZE (sizeof(VOICE_PROCESSED)-4*sizeof(void*))

struct VOICE_PROCESSED
{
	VOICE_PROCESSED()
	{
		memset(this, 0, sizeof(VOICE_PROCESSED));
	}

	void SetVolume(int right);
	void StartSound();
	void VoiceChangeFrequency();
	void InterpolateUp();
	void InterpolateDown();
	void FModChangeFrequency(int ns);
	int iGetNoiseVal();
	void StoreInterpolationVal(int fa);
	int iGetInterpolationVal();
	void Stop();

	SPU_CONTROL_* GetCtrl();

	// start save state
	int leftvol, rightvol;	 // left right volumes

	int iSBPos;							 // mixing stuff
	int SB[32+32];
	int spos;
	int sinc;

	int iIrqDone;						   // debug irq done flag
	int s_1;								// last decoding infos
	int s_2;
	int iOldNoise;						  // old noise val for this channel   
	int			   iActFreq;						   // current psx pitch
	int			   iUsedFreq;						  // current pc pitch

	int iStartAddr, iLoopAddr, iNextAddr;
	int bFMod;

	ADSRInfoEx ADSRX;							  // next ADSR settings (will be moved to active on sample start)
	int memoffset;				  // if first core, 0, if second, 0x400
	int chanid; // channel id

	bool bIgnoreLoop, bNew, bNoise, bReverb, bOn, bStop, bVolChanged;
	bool bVolumeR, bVolumeL;
	
	// end save state

	///////////////////
	// Sound Buffers //
	///////////////////
	u8* pStart;		   // start and end addresses
	u8* pLoop, *pCurr;

	_SPU_VOICE* pvoice;
};

struct AUDIOBUFFER
{
	u8* pbuf;
	u32 len;

	// 1 if new channels started in this packet
	// Variable used to smooth out sound by concentrating on new voices
	u32 timestamp; // in microseconds, only used for time stretching
	u32 avgtime;
	int newchannels;
};

struct ADMA
{
	unsigned short * MemAddr;
	int			  Index;
	int			  AmountLeft;
	int			  Enabled; 
	// used to make sure that ADMA doesn't get interrupted with a writeDMA call
};

extern ADMA Adma4;
extern ADMA Adma7;

struct SPU2freezeData
{
	u32 version;
	u8 spu2regs[0x10000];
	u8 spu2mem[0x200000];
	u16 interrupt;
	int nSpuIrq[2];
	u32 dwNewChannel2[2], dwEndChannel2[2];
	u32 dwNoiseVal;
	int iFMod[NSSIZE];
	u32 MemAddr[2];
	ADMA adma[2];
	u32 Adma4MemAddr, Adma7MemAddr;

	int SPUCycles, SPUWorkerCycles;
	int SPUStartCycle[2];
	int SPUTargetCycle[2];

	int voicesize;
	VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1];
};

#endif /* __SPU2_H__ */
