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

#ifdef _WIN64
#define MUTEX_NAME TEXT("{E88E4EFC-BEDA-463C-AC93-65154B1817FC}")
#else
#define MUTEX_NAME TEXT("{91383FDC-AF5D-4C17-9347-C3186177EAF6}")
#endif

INT WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, INT cmdShow)
{
	// ensure that only one instance is running (but don't fail if the test fails)
	HANDLE mutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	

	HMODULE library;

	if (mutex == NULL || GetLastError() != ERROR_ALREADY_EXISTS)
	{
		/* BEGIN CHILD PROCESS STUFF */
#ifndef _WIN64
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		HANDLE job;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		if (CreateProcess(TEXT("x64\\Topmost.exe"),NULL,NULL,NULL,FALSE,CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB,NULL,NULL,&si,&pi)) {
			job = CreateJobObject(NULL,NULL);
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION exInfo;
			exInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			SetInformationJobObject(job,JobObjectExtendedLimitInformation,&exInfo,sizeof(exInfo));
			AssignProcessToJobObject(job,pi.hProcess);
			ResumeThread(pi.hThread);
		}
#endif
		/* END CHILD PROCESS STUFF */

		// load the hook library
		if ((library = LoadLibrary(TEXT("Hook.dll"))) == NULL) return GetLastError();
		__try
		{
			HOOKPROC callWndProc;
			HHOOK callWndHook;

			// initialize the hook for injecting the "always on top" entry into the system menu
			if ((callWndProc = (HOOKPROC)GetProcAddress(library, "CallWndProc")) == NULL) return GetLastError();
			if ((callWndHook = SetWindowsHookEx(WH_CALLWNDPROC, callWndProc, library, 0)) == NULL) return GetLastError();
			__try
			{
				HOOKPROC getMsgProc;
				HHOOK getMsgHook;

				// initialize the hook for intercepting the entry's command messages
				if ((getMsgProc = (HOOKPROC)GetProcAddress(library, "GetMsgProc")) == NULL) return GetLastError();
				if ((getMsgHook = SetWindowsHookEx(WH_GETMESSAGE, getMsgProc, library, 0)) == NULL) return GetLastError();
				__try
				{
					MSG msg;
					BOOL ret;

					// pump the messages until the end of the session
					while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
					{
						if (ret == -1) return GetLastError();
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				__finally { UnhookWindowsHookEx(getMsgHook); }
			}
			__finally { UnhookWindowsHookEx(callWndHook); }
		}
		__finally { FreeLibrary(library); }
	}

	// return success
	return ERROR_SUCCESS;
}
