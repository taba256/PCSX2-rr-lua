#include "PrecompiledHeader.h"
#include "windows\resource.h"
#include "TAS.h"
#include "Lua.h"
#include "Win32.h"

HWND LuaConsoleHWnd = NULL;
HFONT hFont = NULL;
LOGFONT LuaConsoleLogFont;
HWND hDmyWnd = NULL;
HWND hLuaDlg = NULL;
std::string consoleputstring("");

struct ControlLayoutInfo
{
	int controlID;

	enum LayoutType // what to do when the containing window resizes
	{
		NONE, // leave the control where it was
		RESIZE_END, // resize the control
		MOVE_START, // move the control
	};
	LayoutType horizontalLayout;
	LayoutType verticalLayout;
};
struct ControlLayoutState
{
	int x, y, width, height;
	bool valid;
	ControlLayoutState() : valid(false) {}
};

static ControlLayoutInfo controlLayoutInfos[] = {
	{ IDC_LUACONSOLE, ControlLayoutInfo::RESIZE_END, ControlLayoutInfo::RESIZE_END },
	{ IDC_EDIT_LUAPATH, ControlLayoutInfo::RESIZE_END, ControlLayoutInfo::NONE },
	{ IDC_BUTTON_LUARUN, ControlLayoutInfo::MOVE_START, ControlLayoutInfo::NONE },
	{ IDC_BUTTON_LUASTOP, ControlLayoutInfo::MOVE_START, ControlLayoutInfo::NONE },
};
static const int numControlLayoutInfos = sizeof(controlLayoutInfos) / sizeof(*controlLayoutInfos);

struct {
	int width; int height;
	ControlLayoutState layoutState[numControlLayoutInfos];
} windowInfo;

//  文字列を置換する
std::string Replace(std::string String1, std::string String2, std::string String3)
{
	std::string::size_type  Pos(String1.find(String2));

	while (Pos != std::string::npos)
	{
		String1.replace(Pos, String2.length(), String3);
		Pos = String1.find(String2, Pos + String3.length());
	}

	return String1;
}

/*
	Runボタンを押すと同時にコンソールに出力しようとすると、デッドロックで死にます
	出力したい文字列を一旦保存して、PostMessageを投げて後で出力します
	うっかりSendMessageにするとしんでしまいます^q^
*/
void PrintToWindowConsole(const char* str){
	if (!hLuaDlg)
		return;
	if (consoleputstring.empty())
		PostMessage(hLuaDlg, WM_COMMAND, IDC_LUACONSOLE_UPDATE, 0);
	//consoleputstring += str;
	consoleputstring.append(str);
}
void ClearWindowConsole(){
	if (!hLuaDlg)
		return;
	consoleputstring.clear();
	PostMessage(hLuaDlg, WM_COMMAND, IDC_LUACONSOLE_CLEAR, 0);
}

void WinLuaOnStart()
{
	HWND hDlg = hLuaDlg;
	//LuaPerWindowInfo& info = LuaWindowInfo[hDlg];
	//info.started = true;
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_LUABROWSE), false); // disable browse while running because it misbehaves if clicked in a frameadvance loop
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_LUASTOP), true);
	SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_LUARUN), "Restart");
	SetWindowText(GetDlgItem(hDlg, IDC_LUACONSOLE), ""); // clear the console
	//      Show_Genesis_Screen(HWnd); // otherwise we might never show the first thing the script draws
}

void WinLuaOnStop()
{
	HWND hDlg = hLuaDlg;
	//LuaPerWindowInfo& info = LuaWindowInfo[hDlg];

	HWND prevWindow = GetActiveWindow();
	SetActiveWindow(hDlg); // bring to front among other script/secondary windows, since a stopped script will have some message for the user that would be easier to miss otherwise
	//if (prevWindow == AfxGetMainWnd()->GetSafeHwnd()) SetActiveWindow(prevWindow);

	//info.started = false;
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_LUABROWSE), true);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_LUASTOP), false);
	SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_LUARUN), "Run");
	//      if(statusOK)
	//              Show_Genesis_Screen(MainWindow->getHWnd()); // otherwise we might never show the last thing the script draws
	//if(info.closeOnStop)
	//      PostMessage(hDlg, WM_CLOSE, 0, 0);
}

INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch (msg) {

	case WM_INITDIALOG:
	{
						  // remove the 30000 character limit from the console control
						  SendMessage(GetDlgItem(hDlg, IDC_LUACONSOLE), EM_LIMITTEXT, 0, 0);


						  //GetWindowRect(GetParent(hDlg), &r);
						  GetWindowRect(gApp.hWnd, &r);
						  dx1 = (r.right - r.left) / 2;
						  dy1 = (r.bottom - r.top) / 2;

						  GetWindowRect(hDlg, &r2);
						  dx2 = (r2.right - r2.left) / 2;
						  dy2 = (r2.bottom - r2.top) / 2;

						  int windowIndex = 0;//std::find(LuaScriptHWnds.begin(), LuaScriptHWnds.end(), hDlg) - LuaScriptHWnds.begin();
						  int staggerOffset = windowIndex * 24;
						  r.left += staggerOffset;
						  r.right += staggerOffset;
						  r.top += staggerOffset;
						  r.bottom += staggerOffset;

						  // push it away from the main window if we can
						  const int width = (r.right - r.left);
						  const int width2 = (r2.right - r2.left);
						  if (r.left + width2 + width < GetSystemMetrics(SM_CXSCREEN))
						  {
							  r.right += width;
							  r.left += width;
						  }
						  else if ((int)r.left - (int)width2 > 0)
						  {
							  r.right -= width2;
							  r.left -= width2;
						  }

						  SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);


						  RECT r3;
						  GetClientRect(hDlg, &r3);
						  windowInfo.width = r3.right - r3.left;
						  windowInfo.height = r3.bottom - r3.top;
						  for (int i = 0; i < numControlLayoutInfos; i++) {
							  ControlLayoutState& layoutState = windowInfo.layoutState[i];
							  layoutState.valid = false;
						  }

						  DragAcceptFiles(hDlg, true);
						  SetDlgItemText(hDlg, IDC_EDIT_LUAPATH, PCSX2GetLuaScriptName());

						  SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &LuaConsoleLogFont, 0); // reset with an acceptable font

						  LuaConsoleHWnd = GetDlgItem(hDlg, IDC_LUACONSOLE);
						  consoleputstring.clear();
						  consoleputstring.reserve(250000);
						  return true;
	}       break;

	case WM_SIZE:
	{
					// resize or move controls in the window as necessary when the window is resized

					//LuaPerWindowInfo& windowInfo = LuaWindowInfo[hDlg];
					int prevDlgWidth = windowInfo.width;
					int prevDlgHeight = windowInfo.height;

					int dlgWidth = LOWORD(lParam);
					int dlgHeight = HIWORD(lParam);

					int deltaWidth = dlgWidth - prevDlgWidth;
					int deltaHeight = dlgHeight - prevDlgHeight;

					for (int i = 0; i < numControlLayoutInfos; i++)
					{
						ControlLayoutInfo layoutInfo = controlLayoutInfos[i];
						ControlLayoutState& layoutState = windowInfo.layoutState[i];

						HWND hCtrl = GetDlgItem(hDlg, layoutInfo.controlID);

						int x, y, width, height;
						if (layoutState.valid)
						{
							x = layoutState.x;
							y = layoutState.y;
							width = layoutState.width;
							height = layoutState.height;
						}
						else
						{
							RECT r;
							GetWindowRect(hCtrl, &r);
							POINT p = { r.left, r.top };
							ScreenToClient(hDlg, &p);
							x = p.x;
							y = p.y;
							width = r.right - r.left;
							height = r.bottom - r.top;
						}

						switch (layoutInfo.horizontalLayout)
						{
						case ControlLayoutInfo::RESIZE_END: width += deltaWidth; break;
						case ControlLayoutInfo::MOVE_START: x += deltaWidth; break;
						default: break;
						}
						switch (layoutInfo.verticalLayout)
						{
						case ControlLayoutInfo::RESIZE_END: height += deltaHeight; break;
						case ControlLayoutInfo::MOVE_START: y += deltaHeight; break;
						default: break;
						}

						SetWindowPos(hCtrl, 0, x, y, width, height, 0);

						layoutState.x = x;
						layoutState.y = y;
						layoutState.width = width;
						layoutState.height = height;
						layoutState.valid = true;
					}

					windowInfo.width = dlgWidth;
					windowInfo.height = dlgHeight;

					RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE);
	}       break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL: {
						   EndDialog(hDlg, true); // goto case WM_CLOSE;
		}       break;

		case IDC_BUTTON_LUARUN:
		{
								  if (!g_ReturnToGui)
								  {
									  consoleputstring.clear();
									  char filename[MAX_PATH];
									  GetDlgItemText(hDlg, IDC_EDIT_LUAPATH, filename, MAX_PATH);
									  if(PCSX2LoadLuaCode(filename))
										  WinLuaOnStart();
								  }
		}       break;

		case IDC_BUTTON_LUASTOP:
		{
								   PCSX2LuaStop();
								   WinLuaOnStop();
		}       break;

		case IDC_BUTTON_LUAEDIT:
		{
								   char Str_Tmp[1024];
								   SendDlgItemMessage(hDlg, IDC_EDIT_LUAPATH, WM_GETTEXT, (WPARAM)512, (LPARAM)Str_Tmp);
								   // tell the OS to open the file with its associated editor,
								   // without blocking on it or leaving a command window open.
								   if ((int)ShellExecute(NULL, "edit", Str_Tmp, NULL, NULL, SW_SHOWNORMAL) == SE_ERR_NOASSOC)
								   if ((int)ShellExecute(NULL, "open", Str_Tmp, NULL, NULL, SW_SHOWNORMAL) == SE_ERR_NOASSOC)
									   ShellExecute(NULL, NULL, "notepad", Str_Tmp, NULL, SW_SHOWNORMAL);
		}       break;

		case IDC_BUTTON_LUABROWSE:
		{

									 //systemSoundClearBuffer();

									 //CString filter = winResLoadFilter(IDS_FILTER_LUA);
									 //CString title = winResLoadString(IDS_SELECT_LUA_NAME);

									 //CString luaName = winGetDestFilename(theApp.gameFilename, IDS_LUA_DIR, ".lua");
									 //CString luaDir = winGetDestDir(IDS_LUA_DIR);

									 //filter.Replace('|', '\000');
									 //                              char *p = filter.GetBuffer(0);
									 //                              while ((p = strchr(p, '|')) != NULL)
									 //                                      *p++ = 0;

									 char filenamebuffer[MAX_PATH];
									 ZeroMemory(filenamebuffer, MAX_PATH);
									 OPENFILENAME  ofn;
									 ZeroMemory((LPVOID)&ofn, sizeof(OPENFILENAME));
									 ofn.lpstrFile = filenamebuffer;
									 ofn.nMaxFile = MAX_PATH;
									 ofn.lStructSize = sizeof(OPENFILENAME);
									 ofn.hwndOwner = hDlg;
									 ofn.lpstrFilter = "Lua Script(*.lua)\0*.lua\0All files(*.*)\0*.*\0\0";
									 ofn.nFilterIndex = 0;
									 //ofn.lpstrInitialDir = "";
									 ofn.lpstrTitle = "choose lua file";
									 ofn.lpstrDefExt = "lua";
									 ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER; // hide previously-ignored read-only checkbox (the real read-only box is in the open-movie dialog itself)
									 if (GetOpenFileName(&ofn))
									 {
										 SetWindowText(GetDlgItem(hDlg, IDC_EDIT_LUAPATH), filenamebuffer);
									 }

									 return true;
		}       break;

		case IDC_EDIT_LUAPATH:
		{
								 char filename[MAX_PATH];
								 GetDlgItemText(hDlg, IDC_EDIT_LUAPATH, filename, MAX_PATH);
								 FILE* file = fopen(filename, "rb");
								 EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_LUAEDIT), file != NULL);
								 if (file)
									 fclose(file);
		}       break;

		case IDC_LUACONSOLE_CHOOSEFONT:
		{
										  CHOOSEFONT cf;

										  ZeroMemory(&cf, sizeof(cf));
										  cf.lStructSize = sizeof(CHOOSEFONT);
										  cf.hwndOwner = hDlg;
										  cf.lpLogFont = &LuaConsoleLogFont;
										  cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
										  if (ChooseFont(&cf)) {
											  if (hFont) {
												  DeleteObject(hFont);
												  hFont = NULL;
											  }
											  hFont = CreateFontIndirect(&LuaConsoleLogFont);
											  if (hFont)
												  SendDlgItemMessage(hDlg, IDC_LUACONSOLE, WM_SETFONT, (WPARAM)hFont, 0);
										  }
		}       break;

		case IDC_LUACONSOLE_CLEAR:
		{
									 SetWindowText(GetDlgItem(hDlg, IDC_LUACONSOLE), "");
		}       break;
		case IDC_LUACONSOLE_UPDATE:
		{
									  /*排他制御してやらないと、出力したい文字列が途切れたりしてしまうのです。。。*/
									  lockLuamutex();
									  if (!LuaConsoleHWnd || consoleputstring.empty()){
										  unlockLuamutex();
										  break;
									  }
									  HWND hConsole = LuaConsoleHWnd;

									  int length = GetWindowTextLength(hConsole);
									  if (length >= 250000)
									  {
										  // discard first half of text if it's getting too long
										  SendMessage(hConsole, EM_SETSEL, 0, length / 2);
										  SendMessage(hConsole, EM_REPLACESEL, false, (LPARAM)"");
										  length = GetWindowTextLength(hConsole);
									  }
									  SendMessage(hConsole, EM_SETSEL, length, length);

									  //LuaPerWindowInfo& info = LuaWindowInfo[hDlg];
									  {
										  consoleputstring = Replace(consoleputstring, "\n", "\r\n");
										  SendMessage(hConsole, EM_REPLACESEL, false, (LPARAM)consoleputstring.c_str());
									  }
									  consoleputstring.clear();
									  unlockLuamutex();
		}		break;

		} // switch (LOWORD(wParam))
		break;

	case WM_CLOSE: {
					   //SendMessage(hDlg, WM_DESTROY, 0, 0);
					   DestroyWindow(hDmyWnd);
	}       break;

	case WM_DESTROY: {
						 PCSX2LuaStop();
						 DragAcceptFiles(hDlg, FALSE);
						 if (hFont) {
							 DeleteObject(hFont);
							 hFont = NULL;
						 }
						 LuaConsoleHWnd = NULL;
						 hLuaDlg = NULL;
						 hDmyWnd = NULL;
	}       break;

	case WM_DROPFILES: {
						   HDROP hDrop;
						   //UINT fileNo;
						   UINT fileCount;
						   char filename[_MAX_PATH];

						   hDrop = (HDROP)wParam;
						   fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
						   if (fileCount > 0) {
							   DragQueryFile(hDrop, 0, filename, sizeof(filename));
							   SetWindowText(GetDlgItem(hDlg, IDC_EDIT_LUAPATH), filename);
						   }
						   DragFinish(hDrop);
						   return true;
	}       break;

	}

	return false;

}

/*
	PCSX2-rrではゲームを起動するとメインのメッセージループが止まってしまうので、別スレッドで独自に回してやる必要がある
	別スレッドでダミーのウィンドウを作って、子としてダイアログを作る
	なお別スレッドで動かすため、排他制御やデッドロック対策が必要。Shit!
	*/
pthread_t thread;
void* _CreateLuaWindow(void*param){
	MSG msg;
	hDmyWnd = CreateWindow("STATIC", "dummy", 0, 0, 0, 0, 0, NULL, NULL, gApp.hInstance, 0);
	hLuaDlg = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_LUA), hDmyWnd, DlgLuaScriptDialog);
	while (true)
	{
		BOOL ret = GetMessage(&msg, NULL, 0, 0);
		if (ret == 0 || ret == -1){
			break;
		}
		else{
			if (hLuaDlg == NULL || !IsDialogMessage(hLuaDlg, &msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	hDmyWnd = NULL;
	return NULL;
}

void CreateLuaWindow(){
	if (!hDmyWnd){
		pthread_create(&thread, NULL, _CreateLuaWindow, NULL);
	}
}