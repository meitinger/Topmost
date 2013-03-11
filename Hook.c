/* Copyright (C) 2008-2011, Manuel Meitinger
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Windows.h>

#define COMMAND 0xEFF0
#define MAXLEN  100

LPTSTR GetMenuItemString()
{
	static LONG initialized = 0;
	static TCHAR caption[MAXLEN] = TEXT("Always On &Top");
	HMODULE taskmgr;
	HMENU menu;

	// only try to load the localized string once (thread safe)
	if (!InterlockedBitTestAndSet(&initialized, 0))
	{
		// load the task manager as a resource (don't do this at home kids, erm I mean in a business app)
		if ((taskmgr = LoadLibraryEx(TEXT("taskmgr.exe"), NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE)) != NULL)
		{
			// find the proper menu and get the "always on top" string
			if ((menu = LoadMenu(taskmgr, MAKEINTRESOURCE(106))) != NULL)
			{
				GetMenuString(menu, 40006, caption, MAXLEN, MF_BYCOMMAND);
				DestroyMenu(menu);
			}
			FreeLibrary(taskmgr);
		}
	}

	// return the localized string
	return caption;
}

VOID InsertOrUpdateMenuItem(HWND window)
{
	HMENU menu;
	BOOL isNormal;
	UINT state;
	MENUITEMINFO item;

	// get the system menu handle and the current topmost style
	if ((menu = GetSystemMenu(window, FALSE)) == NULL) return;
	isNormal = (GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0;

	// get the state of the "always on top" entry and...
	if ((state = GetMenuState(menu, COMMAND, MF_BYCOMMAND)) == -1)
	{
		// ...create it if it doesn't exist or...
		item.cbSize = sizeof(item);
		item.fMask = MIIM_CHECKMARKS | MIIM_STATE | MIIM_ID | MIIM_STRING;
		item.hbmpChecked = NULL;
		item.hbmpUnchecked = NULL;
		item.fState = isNormal ? MFS_UNCHECKED : MFS_CHECKED;
		item.wID = COMMAND;
		item.dwTypeData = GetMenuItemString();
		InsertMenuItem(menu, SC_CLOSE, FALSE, &item);
	}
	else
	{
		// ...update the check mark if necessary
		if (((state & MF_CHECKED) == 0) != isNormal)
			CheckMenuItem(menu, COMMAND, MF_BYCOMMAND | (isNormal ? MFS_UNCHECKED : MFS_CHECKED));
	}
}

LRESULT CALLBACK CallWndProc(INT code, WPARAM wParam, LPARAM lParam)
{
#define msg ((PCWPSTRUCT)lParam)
	if (code == HC_ACTION)
	{
		switch (msg->message)
		{
			case WM_ACTIVATE:
				// create the custom system menu before WM_INITMENUPOPUP
				GetSystemMenu(msg->hwnd, FALSE);
				break;

			case WM_INITMENUPOPUP:
				// add or update the "always on top" entry
				if ((BOOL)HIWORD(msg->lParam) == TRUE)
					InsertOrUpdateMenuItem(msg->hwnd);
				break;

			case WM_UNINITMENUPOPUP:
				// remove the "always on top" entry
				if ((HIWORD(msg->lParam) & MF_SYSMENU) != 0)
					DeleteMenu((HMENU)msg->wParam, COMMAND, MF_BYCOMMAND);
				break;
		}
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
#undef msg
}

LRESULT CALLBACK GetMsgProc(INT code, WPARAM wParam, LPARAM lParam)
{
#define msg ((PMSG)lParam)
	if (code == HC_ACTION)
	{
		// update the window's topmost style if the "always on top" entry was clicked
		if (msg->message == WM_SYSCOMMAND && msg->wParam == COMMAND)
			SetWindowPos(msg->hwnd, (GetWindowLongPtr(msg->hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0 ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
#undef msg
}
