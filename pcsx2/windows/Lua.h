#pragma once

#include "PsxCommon.h"

enum LuaCallID
{
	LUACALL_BEFOREEMULATION,
	LUACALL_AFTEREMULATION,
	LUACALL_BEFOREEXIT,

	LUACALL_COUNT
};

void CallRegisteredLuaFunctions(LuaCallID calltype);
int PCSX2LoadLuaCode(const char*luaScriptName);
void PCSX2LuaFrameBoundary(void);
void PCSX2LuaStop();
char*PCSX2GetLuaScriptName();