#pragma once

extern HWND LuaConsoleHWnd;

void PrintToWindowConsole(const char* str);
void ClearWindowConsole();
void WinLuaOnStart();
void WinLuaOnStop();
INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateLuaWindow();