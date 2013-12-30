#include "PrecompiledHeader.h"
#include "LuaDlg.h"
#include "Lua.h"
#include "Lua\lua.hpp"
#include "../TAS.h"
#include <mutex>

//スクリプト名
static char *luaScriptName=nullptr;

//lua実行中？
static bool luaRunning = false;

static lua_State*LUA = NULL;
static int lua_joypads_used = 0;
static unsigned char padData[2][6] = { 0 };
static bool frameBoundary = false;
static bool frameAdvanceWaiting = false;
static int numTries = 0;
static string LuaDirectory = "";
static string EmuDirectory = "";
static const char *luaCallIDStrings[] = {
	"CALL_BEFOREEMULATION",
	"CALL_AFTEREMULATION",
	"CALL_BEFOREEXIT",
	"CALL_MEMORYWRITE"
};
static const char *button_mappings[] = {
	"select", "l3", "r3", "start", "up", "right", "down", "left",
	"l2", "r2", "l1", "r1", "triangle", "circle", "cross", "square"
};
static const char *analog_mappings[] = {
	"rx", "ry", "lx", "ly"
};

static const char *frameAdvanceThread = "PCSX2.FrameAdvance";
static std::recursive_timed_mutex scriptExecMtx;

/*emu/pcsx2 functions*/
//print
static int print(lua_State*L){
	string str = lua_tostring(L, 1);
	PrintToWindowConsole(str.c_str());
	return 0;
}
//clear
static int consoleClear(lua_State*L){
	ClearWindowConsole();
	return 0;
}
//getframecount
static int L_getFrameCount(lua_State*L){
	lua_pushinteger(L, g_Movie.FrameNum);
	return 1;
}
//pause
static int L_emupause(lua_State*L){
	g_Movie.Paused = !!luaL_checkint(L, 1);
	return 0;
}
//frameadvance
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
//register関連
static int registerfunction(lua_State*L, LuaCallID type){
	if (!lua_isnil(L, 1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_settop(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[type]);
	lua_insert(L, 1);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[type]);
	return 1;
}
static int L_registerbefore(lua_State*L){
	return registerfunction(L, LUACALL_BEFOREEMULATION);
}
static int L_registerafter(lua_State*L){
	return registerfunction(L, LUACALL_AFTEREMULATION);
}
static int L_registerexit(lua_State*L){
	return registerfunction(L, LUACALL_BEFOREEXIT);
}

/*memory read/write functions*/
static u32 readMemory(lua_State*L, int size){
	int addr = luaL_checkint(L, 1);
	if (!PS2MEM_BASE || addr<0 || addr>(0x2000000 - size)){
		return 0;
	}
	u32 out = 0;
	out = *(u32*)(PS2MEM_BASE + addr);
	return out;
}
static int L_getDWORD(lua_State*L){
	u32 m;
	m = readMemory(L, 4);
	lua_pushnumber(L, m);
	return 1;
}
static int L_getWORD(lua_State*L){
	u16 m;
	m = readMemory(L, 2);
	lua_pushinteger(L, m);
	return 1;
}
static int L_getBYTE(lua_State*L){
	u8 m;
	m = readMemory(L, 1);
	lua_pushinteger(L, m);
	return 1;
}
static int L_getDWORDsigned(lua_State*L){
	s32 m;
	m = readMemory(L, 4);
	lua_pushinteger(L, m);
	return 1;
}
static int L_getWORDsigned(lua_State*L){
	s16 m;
	m = readMemory(L, 2);
	lua_pushinteger(L, m);
	return 1;
}
static int L_getBYTEsigned(lua_State*L){
	s8 m;
	m = readMemory(L, 1);
	lua_pushinteger(L, m);
	return 1;
}
static int L_getfloat(lua_State*L){
	int addr = luaL_checkint(L, 1);
	if (!PS2MEM_BASE || addr<0 || addr>(0x2000000 - 4)){
		return 0;
	}
	float out = 0;
	out = *(float*)(PS2MEM_BASE + addr);
	lua_pushnumber(L, out);
	return 1;
}
static int L_getbyterange(lua_State*L){
	int addr = luaL_checkint(L, 1);
	if (!PS2MEM_BASE || addr<0 || addr>(0x2000000 - 4)){
		return 0;
	}
	int length = luaL_checkint(L, 2);
	if (length < 0){
		addr += length;
		length *= -1;
	}
	lua_pushlstring(L, (char*)(PS2MEM_BASE + addr), length);
	return 1;
}
static int writeMemory(lua_State*L, int size){
	int addr = luaL_checkint(L, 1);
	if (!PS2MEM_BASE || addr<0 || addr>(0x2000000 - size)){
		return 0;
	}
	int src = luaL_checkunsigned(L, 2);
	memcpy(PS2MEM_BASE + addr, &src, size);
	return 0;
}
static int L_setDWORD(lua_State*L){
	return writeMemory(L, 4);
}
static int L_setWORD(lua_State*L){
	return writeMemory(L, 2);
}
static int L_setBYTE(lua_State*L){
	return writeMemory(L, 1);
}
static int L_setfloat(lua_State*L){
	int addr = luaL_checkint(L, 1);
	if (!PS2MEM_BASE || addr<0 || addr>(0x2000000 - 4)){
		return 0;
	}
	float src = luaL_checknumber(L, 2);
	*(float*)(PS2MEM_BASE + addr) = src;
	return 0;
}

/*Joypad functions*/
int L_getJoyPad(lua_State*L){
	int which = luaL_checkint(L, 1);
	--which;
	if (which<0 || which>1){
		return 0;
	}
	lua_newtable(L);
	u16 buttons = g_PadData[which][2] | (g_PadData[which][3] << 8);
	buttons = ~buttons;//マイナス形式なのでbit反転
	for (int i=0; buttons; buttons >>= 1,i++){
		if (buttons & 1){
			lua_pushboolean(L, 1);
			lua_setfield(L, -2, button_mappings[i]);
		}
	}
	for (int i = 0; i < 4; i++){
		lua_pushinteger(L, g_PadData[which][4+i]);
		lua_setfield(L, -2, analog_mappings[i]);
	}
	return 1;
}
int L_setJoyPad(lua_State*L){
	int which = luaL_checkint(L, 1);
	--which;
	if (which<0 || which>1){
		return 0;
	}
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_joypads_used |= 1 << which;
	u16 buttons = 0;
	u8 analog[4] = {127,127,127,127};
	for (int i = 0; i < 16; i++){
		lua_getfield(L, 2, button_mappings[i]);
		if (!lua_isnil(L, -1)){
			bool pressed = lua_toboolean(L, -1) != 0;
			if (pressed)
				buttons |= 1 << i;
			else
				buttons &= ~(1 << i);
		}
		lua_pop(L, 1);
	}
	buttons = ~buttons;
	padData[which][0] = buttons & 0xff;
	padData[which][1] = (buttons >> 8) & 0xff;
	//analog
	for (int i = 0; i < 4; i++){
		lua_getfield(L, 2, analog_mappings[i]);
		if (!lua_isnil(L, -1)){
			unsigned char ana = lua_tointeger(L, -1) != 0;
			padData[which][2 + i] = ana;
		}
		else{
			padData[which][2 + i] = 127;
		}
		lua_pop(L, 1);
	}
	return 0;
}

/*experimental functions*/
int L_trayopen(lua_State*L){
	lua_pushinteger(L, cdvdCtrlTrayOpen());
	return 1;
}
int L_trayclose(lua_State*L){
	lua_pushinteger(L, cdvdCtrlTrayClose());
	return 1;
}
int L_traystatus(lua_State*L){
	lua_pushinteger(L, cdvdGetTrayStatus());
	return 1;
}
static int memwriteaddr = 0;
bool Lua_memwriteAddrCheck(int addr){
	if (memwriteaddr != 0 && memwriteaddr == addr)
		return true;
	return false;
}
static int L_registermemwrite(lua_State*L){
	memwriteaddr = luaL_checkint(L, 1);
	lua_remove(L, 1);
	return registerfunction(L, LUACALL_MEMORYWRITE);
}
static int L_getRegister(lua_State*L){
	int reg = luaL_checkint(L, 1) & 0x1f;
	lua_pushunsigned(L, cpuRegs.GPR.r[reg].UL[0]);
	return 1;
}
static int L_getPC(lua_State*L){
	lua_pushunsigned(L, cpuRegs.pc);
	return 1;
}

static int printerror(lua_State *L, int idx)
{
	PrintToWindowConsole(lua_tostring(L, lua_gettop(L)));
	return 0;
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

//PCSX2LoadLuaCodeでLua側に登録します
static const struct luaL_Reg emufunctions[] = {
	{ "print", print },
	{ "clear", consoleClear },
	{ "frameadvance", pcsx2_frameadvance },
	{ "getframecount", L_getFrameCount },
	{ "pause", L_emupause },
	{ "registerbefore", L_registerbefore },
	{ "registerafter", L_registerafter },
	{ "registerexit", L_registerexit },
	{ NULL, NULL }
};
static const struct luaL_Reg memoryfunctions[] = {
	{ "readdword", L_getDWORD },
	{ "readdwordunsigned", L_getDWORD },
	{ "readlong", L_getDWORD },
	{ "readlongunsigned", L_getDWORD },
	{ "readword", L_getWORD },
	{ "readwordunsigned", L_getWORD },
	{ "readshort", L_getWORD },
	{ "readshortunsigned", L_getWORD },
	{ "readbyte", L_getBYTE },
	{ "readbyteunsigned", L_getBYTE },
	{ "readdwordsigned", L_getDWORDsigned },
	{ "readlongsigned", L_getDWORDsigned },
	{ "readwordsigned", L_getWORDsigned },
	{ "readshortsigned", L_getWORDsigned },
	{ "readbytesigned", L_getBYTEsigned },
	{ "readfloat", L_getfloat },
	{ "readbyterange", L_getbyterange },
	{ "writedword", L_setDWORD },
	{ "writelong", L_setDWORD },
	{ "writeword", L_setWORD },
	{ "writeshort", L_setWORD },
	{ "writebyte", L_setBYTE },
	{ "writefloat", L_setfloat },
	{ NULL, NULL }
};
static const struct luaL_Reg joypadfunctions[] = {
	{ "get", L_getJoyPad },
	{ "set", L_setJoyPad },
	{ NULL, NULL }
};
static const struct luaL_Reg experimentalfunctions[] = {
	{ "trayopen", L_trayopen },
	{ "trayclose", L_trayclose },
	{ "traystatus", L_traystatus },
	{ "registerwrite", L_registermemwrite },
	{ "readreg", L_getRegister },
	{ "readpc", L_getPC },
	{ NULL, NULL }
};

void PCSX2LuaFrameBoundary(void)
{
	std::lock_guard<std::recursive_timed_mutex> lock(scriptExecMtx);
	// printf("Lua Frame\n");

	lua_joypads_used = 0;

	// HA!
	if (!LUA || !luaRunning){
		return;
	}

	SetCurrentDirectory(LuaDirectory.c_str());

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
	SetCurrentDirectory(EmuDirectory.c_str());
}

// for Lua 5.2 or newer
static void luaL_register(lua_State*L, const char*n, const luaL_Reg*l){
	lua_newtable(L);
	for (int i = 0; l[i].func != NULL&&l[i].name != NULL; i++){
		lua_pushcfunction(L, l[i].func);
		lua_setfield(L, 1, l[i].name);
	}
	lua_setglobal(L, n);
}

int PCSX2LoadLuaCode(const char*filename){
	std::lock_guard<std::recursive_timed_mutex> lock(scriptExecMtx);

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
		//SetCurrentDirectory(dir);
		LuaDirectory = dir;
		GetCurrentDirectory(_MAX_PATH, dir);
		EmuDirectory = dir;
	}
	if (!LUA)
	{
		LUA = luaL_newstate();
		luaL_openlibs(LUA);

		luaL_register(LUA, "emu", emufunctions);
		luaL_register(LUA, "pcsx2", emufunctions);
		luaL_register(LUA, "memory", memoryfunctions);
		//luaL_register(LUA, "savestate", statefunctions);
		luaL_register(LUA, "joypad", joypadfunctions);
		//luaL_register(LUA, "avi", recavifunctions);
		luaL_register(LUA, "exp", experimentalfunctions);
	}

	lua_State *thread = lua_newthread(LUA);
	int result = luaL_loadfile(LUA, filename);
	if (result)
	{
		printerror(LUA, -1);

		// Wipe the stack. Our thread
		lua_settop(LUA, 0);
		return 0; // Oh shit.
	}
	lua_xmove(LUA, thread, 1);
	lua_setfield(LUA, LUA_REGISTRYINDEX, frameAdvanceThread);
	luaRunning = true;
	lua_joypads_used = 0;

	//WinLuaOnStart();

	PCSX2LuaFrameBoundary();

	return 1;
}

void PCSX2LuaStop(){
	std::lock_guard<std::recursive_timed_mutex> lock(scriptExecMtx);

	if (LUA){
		//execute the user's shutdown callbacks
		CallRegisteredLuaFunctions(LUACALL_BEFOREEXIT);
		lua_close(LUA);
		LUA = NULL;
		memwriteaddr = 0;
		PCSX2LuaOnStop();
	}
	//WinLuaOnStop();
}

void CallRegisteredLuaFunctions(LuaCallID calltype)
{
	std::lock_guard<std::recursive_timed_mutex> lock(scriptExecMtx);
	assert((unsigned int)calltype < (unsigned int)LUACALL_COUNT);

	const char *idstring = luaCallIDStrings[calltype];

	if (!LUA)
		return;

	SetCurrentDirectory(LuaDirectory.c_str());

	lua_settop(LUA, 0);
	lua_getfield(LUA, LUA_REGISTRYINDEX, idstring);

	int errorcode = 0;
	if (lua_isfunction(LUA, -1))
	{
		errorcode = lua_pcall(LUA, 0, 0, 0);
		if (errorcode){
			//HandleCallbackError(LUA);
			printerror(LUA, -1);
			PCSX2LuaStop();
		}
	}
	else
	{
		lua_pop(LUA, 1);
	}
	SetCurrentDirectory(EmuDirectory.c_str());
}

char*PCSX2GetLuaScriptName(){
	return luaScriptName;
}

void lockLuamutex(){
	scriptExecMtx.lock();
}

void unlockLuamutex(){
	scriptExecMtx.unlock();
}

void Luajoypadset(){
	if (g_Movie.File && g_Movie.Replay)
		return;
	if (!lua_joypads_used)
		return;
	if (lua_joypads_used & 1){
		memcpy(g_PadData[0] + 2, padData[0], 6);
	}
	if (lua_joypads_used & 2){
		memcpy(g_PadData[1] + 2, padData[1], 6);
	}
}

bool usingjoypad(int port){
	if (g_Movie.File && g_Movie.Replay)
		return false;
	return !!(lua_joypads_used&(1<<port));
}