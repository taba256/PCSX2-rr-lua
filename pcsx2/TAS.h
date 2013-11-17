#ifndef __TAS_H__
#define __TAS_H__

#include <stdio.h>
#include "PsxCommon.h"

struct MovieInfo {
	FILE* File;
	u32   FrameNum;
	u32   FrameMax;
	u32   Rerecs;
	bool  Paused;
	bool  Replay;
	bool  ReadOnly;
	char  Filename[256];
	cdvdRTC BootTime;
};

extern MovieInfo g_Movie;
extern u8 g_PadData[2][20];

void MovieEnd();
bool MovieRecord(char*);
bool MoviePlay(char*);

#endif
