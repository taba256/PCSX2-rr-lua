#pragma once

extern HWND LuaConsoleHWnd;

void PrintToWindowConsole(const char* str);
void WinLuaOnStart(int hDlgAsInt);
void WinLuaOnStop(int hDlgAsInt);
INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateLuaWindow();