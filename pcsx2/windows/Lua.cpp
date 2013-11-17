#include "PrecompiledHeader.h"
#include "LuaDlg.h"
#include "Lua.h"
#include "Lua\lua.hpp"

//スクリプト名
static char *luaScriptName=nullptr;

//lua実行中？
static bool luaRunning = false;

lua_State*LUA = nullptr;
int lua_joypads_used = 0;
static bool frameBoundary = false;
static bool frameAdvanceWaiting = false;
int numTries = 0;

static const char *frameAdvanceThread = "PCSX2.FrameAdvance";

bool DemandLua()
{
	return true;
#ifdef _WIN32
	HMODULE mod = LoadLibrary("lua52.dll");
	if (!mod)
	{
		MessageBox(NULL, "lua52.dll was not found. Please get it into your PATH or in the same directory as PCSX2.exe", "PCSX2", MB_OK | MB_ICONERROR);
		return false;
	}
	FreeLibrary(mod);
	return true;
#else
	return true;
#endif
}

static int L_print(lua_State*L){
	string str = lua_tostring(L, 1);
	PrintToWindowConsole(str.c_str());
	return 0;
}

static int printerror(lua_State *L, int idx)
{
	PrintToWindowConsole(lua_tostring(L, lua_gettop(L)));
	return 0;
}

static int pcsx2_frameadvance(lua_State *L)
{
	// We're going to sleep for a frame-advance. Take notes.
	if (frameAdvanceWaiting)
		return luaL_error(L, "can't call pcsx2.frameadvance() from here");

	frameAdvanceWaiting = true;

	// Don't do this! The user won't like us sending their emulator out of control!
	// Settings.FrameAdvance = true;
	// Now we can yield to the main
	return lua_yield(L, 0);

	// It's actually rather disappointing...
}

/**
* Resets emulator speed / pause states after script exit.
*/
static void PCSX2LuaOnStop(void)
{
	luaRunning = false;
	lua_joypads_used = 0;
	//if (wasPaused)
	// systemSetPause(true);
}

static const struct luaL_Reg emufunctions[] = {
	{ "print", L_print },
//	{ "cls", L_clearConsole },
	{ "frameadvance", pcsx2_frameadvance },
//	{ "getframecount", L_getFrameCount },
//	{ "pause", L_emupause },
//	{ "registerbefore", L_registerbefore },
//	{ "registerafter", L_registerafter },
//	{ "registerexit", L_registerexit },
	{ NULL, NULL }
};

void PCSX2LuaFrameBoundary(void)
{
	// printf("Lua Frame\n");

	lua_joypads_used = 0;

	// HA!
	if (!LUA || !luaRunning)
		return;

	// Our function needs calling
	lua_settop(LUA, 0);
	lua_getfield(LUA, LUA_REGISTRYINDEX, frameAdvanceThread);

	lua_State *thread = lua_tothread(LUA, 1);

	// Lua calling C must know that we're busy inside a frame boundary
	frameBoundary = true;
	frameAdvanceWaiting = false;

	numTries = 1000;

	int result = lua_resume(thread,NULL, 0);

	if (result == LUA_YIELD)
	{
		// Okay, we're fine with that.
	}
	else if (result != 0)
	{
		// Done execution by bad causes
		PCSX2LuaOnStop();
		lua_pushnil(LUA);
		lua_setfield(LUA, LUA_REGISTRYINDEX, frameAdvanceThread);
		//lua_pushnil(LUA);
		//lua_setfield(LUA, LUA_REGISTRYINDEX, guiCallbackTable);

		// Error?
		//#if (defined(WIN32) && !defined(SDL))
		// info_print(info_uid, lua_tostring(thread, -1)); //Clear_Sound_Buffer();
		// AfxGetApp()->m_pMainWnd->MessageBox(lua_tostring(thread, -1), "Lua run error", MB_OK | MB_ICONSTOP);
		//#else
		// fprintf(stderr, "Lua thread bombed out: %s\n", lua_tostring(thread, -1));
		//#endif
		printerror(thread, -1);
	}
	else
	{
		PCSX2LuaOnStop();
		printf("Script died of natural causes.\n");
	}

	// Past here, VBA actually runs, so any Lua code is called mid-frame. We must
	// not do anything too stupid, so let ourselves know.
	frameBoundary = false;

	if (!frameAdvanceWaiting)
	{
		PCSX2LuaOnStop();
	}
}

// for Lua 5.2 or newer
void luaL_register(lua_State*L, const char*n, const luaL_Reg*l){
	lua_newtable(L);
	for (int i = 0; l[i].func != NULL&&l[i].name != NULL; i++){
		lua_pushcfunction(L, l[i].func);
		lua_setfield(L, 1, l[i].name);
	}
	lua_setglobal(L, n);
}

int PCSX2LoadLuaCode(const char*filename){
	if (!DemandLua())
	{
		return 0;
	}

	if (filename != luaScriptName)
	{
		if (luaScriptName)
			free(luaScriptName);
		luaScriptName = strdup(filename);
	}

	//stop any lua we might already have had running
	PCSX2LuaStop();

	// Set current directory from filename (for dofile)
	char dir[_MAX_PATH];
	char *slash, *backslash;
	strcpy(dir, filename);
	slash = strrchr(dir, '/');
	backslash = strrchr(dir, '\\');
	if (!slash || (backslash && backslash < slash))
		slash = backslash;
	if (slash)
	{
		slash[1] = '\0'; // keep slash itself for some reasons
		//chdir(dir);
		SetCurrentDirectory(dir);
	}
	if (!LUA)
	{
		LUA = luaL_newstate();
		luaL_openlibs(LUA);

		luaL_register(LUA, "emu", emufunctions);
		//luaL_register(LUA, "memory", memoryfunctions);
		//luaL_register(LUA, "savestate", statefunctions);
		//luaL_register(LUA, "joypad", joypadfunctions);
		//luaL_register(LUA, "avi", recavifunctions);
	}

	lua_State *thread = lua_newthread(LUA);
	int result = luaL_loadfile(LUA, filename);
	if (result)
	{
		//printerror(LUA, -1);

		// Wipe the stack. Our thread
		lua_settop(LUA, 0);
		return 0; // Oh shit.
	}
	lua_xmove(LUA, thread, 1);
	lua_setfield(LUA, LUA_REGISTRYINDEX, frameAdvanceThread);
	luaRunning = true;
	lua_joypads_used = 0;

	PCSX2LuaFrameBoundary();
	
	return 1;
}

void PCSX2LuaStop(){
	if (!DemandLua())
		return;

	if (LUA){
		//execute the user's shutdown callbacks
		//CallExitFunction();
		lua_close(LUA);
		LUA = nullptr;
		PCSX2LuaOnStop();
	}
}

char*PCSX2GetLuaScriptName(){
	return luaScriptName;
}