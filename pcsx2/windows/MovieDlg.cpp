#include "PrecompiledHeader.h"
#include "Win32.h"
#include "TAS.h"
#include "MovieDlg.h"
//#include "Common.h"
#include <Commctrl.h>

char g_Filename[256];

void OpenMovie(HWND hDlg, BOOL Create) {
	char CurDir[256];
	OPENFILENAME f;
	memset(&f, 0, sizeof(f));
	*g_Filename = 0;
	
	f.lStructSize = sizeof(f);
	f.hwndOwner = hDlg;
	f.lpstrFilter = "PS2 Movie File (*.p2m)\0*.p2m\0\0";
	f.lpstrFile = g_Filename;
	f.nMaxFile = 255;
	f.Flags = OFN_HIDEREADONLY;
	if(Create)
		f.Flags |= OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
	else
		f.Flags |= OFN_FILEMUSTEXIST;

	GetCurrentDirectory(255, CurDir);
	if(GetOpenFileName(&f)) {
		SetDlgItemText(hDlg, IDC_MOVIE_FILE, g_Filename);
		if(!Create) {
			FILE* mfile;
			if(mfile = fopen(g_Filename, "rb")) {
				int x;
				char tmp[256];
				fread(&x, 4, 1, mfile);
				sprintf(tmp, "Frames: %d", x);
				SetDlgItemText(hDlg, IDC_MOVIE_INFO1, tmp);
				fread(&x, 4, 1, mfile);
				sprintf(tmp, "Re-records: %d", x);
				SetDlgItemText(hDlg, IDC_MOVIE_INFO2, tmp);
				cdvdRTC rtc;
				fread(&rtc, sizeof(cdvdRTC), 1, mfile);
				sprintf(tmp, "%d/%02d/%02d %02d:%02d:%02d", rtc.year + 2000, rtc.month, rtc.day,
					rtc.hour - 1, rtc.minute, rtc.second);
				SetDlgItemText(hDlg, IDC_MOVIE_INFO3, tmp);
				fclose(mfile);
			}
		}
	}
	SetCurrentDirectory(CurDir);
}

LRESULT WINAPI RecordDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			Static_SetText(GetDlgItem(hDlg, IDC_MOVIE_FILE), ".p2m");
			SYSTEMTIME st;
			GetSystemTime(&st);
			DateTime_SetSystemtime(GetDlgItem(hDlg, IDC_MOVIE_TIME1), GDT_VALID, &st);
			DateTime_SetSystemtime(GetDlgItem(hDlg, IDC_MOVIE_TIME2), GDT_VALID, &st);
			return TRUE;

		case WM_COMMAND:
			switch(wParam) {
				case IDOK:
					SYSTEMTIME st;
					SendMessage(GetDlgItem(hDlg, IDC_MOVIE_TIME1), DTM_GETSYSTEMTIME, NULL, (LPARAM)&st);
					g_Movie.BootTime.year = st.wYear - 2000;
					g_Movie.BootTime.month = st.wMonth;
					g_Movie.BootTime.day = st.wDay;
					SendMessage(GetDlgItem(hDlg, IDC_MOVIE_TIME2), DTM_GETSYSTEMTIME, NULL, (LPARAM)&st);
					g_Movie.BootTime.hour = (st.wHour + 1) % 24;
					g_Movie.BootTime.minute = st.wMinute;
					g_Movie.BootTime.second = st.wSecond;
					GetDlgItemText(hDlg, IDC_MOVIE_FILE, g_Filename, 255);
					if(MovieRecord(g_Filename)) {
						g_Movie.Paused = (IsDlgButtonChecked(hDlg, IDC_MOVIE_PAUSED) == BST_CHECKED);
						EndDialog(hDlg, TRUE);
					}
					// else error
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, FALSE);
					return TRUE;

				case IDC_MOVIE_OPEN:
					OpenMovie(hDlg, TRUE);
					return TRUE;
			}
			return FALSE;

		case WM_CLOSE:
			EndDialog(hDlg, FALSE);
			return TRUE;
	}
	return FALSE;
}

LRESULT WINAPI PlayDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(wParam) {
				case IDOK:
					GetDlgItemText(hDlg, IDC_MOVIE_FILE, g_Filename, 255);
					if(MoviePlay(g_Filename)) {
						g_Movie.Paused = (IsDlgButtonChecked(hDlg, IDC_MOVIE_PAUSED) == BST_CHECKED);
						g_Movie.ReadOnly = (IsDlgButtonChecked(hDlg, IDC_MOVIE_READONLY) == BST_CHECKED);
						EndDialog(hDlg, TRUE);
					}
					// else error
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, FALSE);
					return TRUE;

				case IDC_MOVIE_OPEN:
					OpenMovie(hDlg, FALSE);
					return TRUE;
			}
			return FALSE;

		case WM_CLOSE:
			EndDialog(hDlg, FALSE);
			return TRUE;
	}
	return FALSE;
}