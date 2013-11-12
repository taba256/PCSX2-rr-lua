#include <string.h>
#include "PrecompiledHeader.h"
#include "System.h"
#include "TAS.h"

MovieInfo g_Movie;
u8 g_PadData[2][20];

void MovieEnd() {
	if(g_Movie.File) {
		if(!g_Movie.ReadOnly) {
			fseek(g_Movie.File, 0, SEEK_SET);
			fwrite(&g_Movie.FrameMax, 4, 1, g_Movie.File);
			fwrite(&g_Movie.Rerecs, 4, 1, g_Movie.File);
		}
		fclose(g_Movie.File);
		g_Movie.File = 0;
		g_Movie.Replay = false;
		if(g_Movie.Replay)
			Console::Notice("Recording ended");
		else
			Console::Notice("Playback ended");
		Console::SetTitle("No Movie");
	}
}

bool MovieRecord(char* File) {
	MovieEnd();
	if(g_Movie.File = fopen(File, "wb+")) {
		setvbuf(g_Movie.File, NULL, _IONBF, 0);
		strcpy(g_Movie.Filename, File);
		g_Movie.FrameMax = 0;
		g_Movie.Rerecs = 0;
		fwrite(&g_Movie.FrameMax, 4, 1, g_Movie.File);
		fwrite(&g_Movie.Rerecs, 4, 1, g_Movie.File);
		g_Movie.Replay = false;
		g_Movie.ReadOnly = false;
		SysClose();
		SysInit();
		SysReset();
		Console::Notice("Recording started: %s", params File);
		return true;
	}
	return false;
}

bool MoviePlay(char* File) {
	MovieEnd();
	if(g_Movie.File = fopen(File, "rb+")) {
		setvbuf(g_Movie.File, NULL, _IONBF, 0);
		strcpy(g_Movie.Filename, File);
		fread(&g_Movie.FrameMax, 4, 1, g_Movie.File);
		fread(&g_Movie.Rerecs, 4, 1, g_Movie.File);
		g_Movie.Replay = true;
		SysClose();
		SysInit();
		SysReset();
		Console::Notice("Playback started: %s", params File);
		return true;
	}
	return false;
}

void SaveState::movieFreeze() {
	if( g_Movie.File ) {
		char* MovieData;
		Freeze(g_Movie.FrameNum);
		u32 MovieSize = g_Movie.FrameNum*6*2;

		if( IsLoading() ) {
			if( !g_Movie.ReadOnly ) {
				MovieData = (char*)malloc(MovieSize);
				rewind(g_Movie.File);
				fwrite(&g_Movie.FrameNum, 4, 1, g_Movie.File);
				fwrite(&g_Movie.Rerecs, 4, 1, g_Movie.File);
				FreezeMem(MovieData, MovieSize);
				fwrite(MovieData, MovieSize, 1, g_Movie.File);
				free(MovieData);
				g_Movie.Replay = false;
				g_Movie.Rerecs++;
			} else {
				fseek(g_Movie.File, 8+MovieSize, SEEK_SET);
				g_Movie.Replay = true;
			}
		} else {
			MovieData = (char*)malloc(MovieSize);
			rewind(g_Movie.File);
			fwrite(&g_Movie.FrameMax, 1, 4, g_Movie.File);
			fwrite(&g_Movie.Rerecs, 1, 4, g_Movie.File);
			fread(MovieData, 1, MovieSize, g_Movie.File);
			FreezeMem(MovieData, MovieSize);
			free(MovieData);
		}
	}
}