#include "Global.h"
#include "InputManager.h"
#include "DeviceEnumerator.h"
#include "resource.h"
#include "KeyboardQueue.h"
#include <math.h>
#include <commctrl.h>

Device *dev;

INT_PTR CALLBACK DiagDialog(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam) {
	int i;
	HWND hWndList = GetDlgItem(hWnd, IDC_LIST);
	static int fullRefresh;
	if (dev) {
		switch (msg) {
		case WM_INITDIALOG:
			{
				fullRefresh = 1;
				SetWindowText(hWnd, dev->displayName);
				LVCOLUMNW c;
				c.mask = LVCF_TEXT | LVCF_WIDTH;
				c.cx = 151;
				c.pszText = L"Control";
				ListView_InsertColumn(hWndList, 0, &c);
				c.cx = 90;
				c.pszText = L"Value";
				ListView_InsertColumn(hWndList, 1, &c);
				ListView_DeleteAllItems(hWndList);
				LVITEM item;
				item.mask = LVIF_TEXT;
				item.iSubItem = 0;
				for (i=0; i<dev->numVirtualControls; i++) {
					item.pszText = dev->GetVirtualControlName(dev->virtualControls+i);
					item.iItem = i;
					ListView_InsertItem(hWndList, &item);
				}
				SetTimer(hWnd, 1, 200, 0);
			}
			//break;
		case WM_TIMER:
			{
				InitInfo info = {0, hWnd, hWnd, hWndList};
				dm->Update(&info);
				LVITEMW item;
				item.mask = LVIF_TEXT;
				item.iSubItem = 1;
				//ShowWindow(hWndList, 0);
				//LockWindowUpdate(hWndList);
				if (!dev->active) {
					item.pszText = L"?";
					for (i=0; i<dev->numVirtualControls; i++) {
						item.iItem = i;
						ListView_SetItem(hWndList, &item);
					}
					fullRefresh = 1;
				}
				else {
					for (i=0; i<dev->numVirtualControls; i++) {
						if (fullRefresh || dev->virtualControlState[i] != dev->oldVirtualControlState[i]) {

							VirtualControl *c = dev->virtualControls + i;
							wchar_t temp[50];
							int val = dev->virtualControlState[i];
							if (c->uid & (UID_POV)) {
								wsprintfW(temp, L"%i", val);
							}
							else {
								wchar_t *sign = L"";
								if (val < 0) {
									sign = L"-";
									val = -val;
								}
								if ((c->uid& UID_AXIS) && val) {
									val = val;
								}
								val = (int)floor(0.5 + val * 1000.0 / (double)FULLY_DOWN);
								wsprintfW(temp, L"%s%i.%03i", sign, val/1000, val%1000);
							}
							item.pszText = temp;
							item.iItem = i;
							ListView_SetItem(hWndList, &item);
						}
					}
					dm->PostRead();
					fullRefresh = 0;
				}
				//LockWindowUpdate(0);
				//ShowWindow(hWndList, 1);
				//UpdateWindow(hWnd);
			}
			break;
		case WM_NOTIFY:
			if (1) {
				PSHNOTIFY* n = (PSHNOTIFY*) lParam;
				if (n->hdr.idFrom != IDC_LIST || n->hdr.code != LVN_KEYDOWN) break;
				NMLVKEYDOWN *key = (NMLVKEYDOWN *) n;
				if (key->wVKey != VK_ESCAPE) break;
			}
			else {
		case WM_ACTIVATE:
				if (wParam != WA_INACTIVE) break;
			}
		case WM_CLOSE:
			KillTimer(hWnd, 1);
			dm->ReleaseInput();
			// Prevents reaching this branch again.
			dev = 0;
			EndDialog(hWnd, 1);
			break;
		default:
			break;
		}
	}
	return 0;
}

void Diagnose(int id, HWND hWnd) {
	// init = 0;
	dev = dm->devices[id];
	for (int i=0; i<dm->numDevices; i++) {
		if (i != id) dm->DisableDevice(i);
		// Shouldn't be needed.
		else dm->EnableDevice(i);
	}
	DialogBox(hInst, MAKEINTRESOURCE(IDD_DIAG), hWnd, DiagDialog);
	ClearKeyQueue();
}
