/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "kill.h"

#if !HAVE_KILL && HAVE_OPENPROCESS

#if HAVE_WINDOWS_H
#include <windows.h>
#endif

#if HAVE_PSAPI_H
#include <psapi.h>
#endif

static void
GetProcessName(pid_t pid, char *ProcessName)
{
    strcpy(ProcessName, "unknown");
    /* Get a handle to the process. */
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    /* Get the process name. */
    if (hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
            GetModuleBaseName(hProcess, hMod, ProcessName, sizeof(ProcessName));
        else
        {
            CloseHandle(hProcess);
            return;
        }
    }
    else
        return;
    CloseHandle(hProcess);
}

SQUIDCEXTERN int
kill(pid_t pid, int sig)
{
    HANDLE hProcess;
    char MyProcessName[MAX_PATH];
    char ProcessNameToCheck[MAX_PATH];

    if (sig == 0)
    {
        if (!(hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)))
            return -1;
        else
        {
            CloseHandle(hProcess);
            GetProcessName(getpid(), MyProcessName);
            GetProcessName(pid, ProcessNameToCheck);
            if (strcmp(MyProcessName, ProcessNameToCheck) == 0)
                return 0;
            return -1;
        }
    }
    else
        return 0;
}

#endif /* !HAVE_KILL && HAVE_OPENPROCESS */
