// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include <stdlib.h>

#define SALADMIN_PROTOCOL_VERSION 1

static BOOL IsAllowedVerb(const wchar_t* verb)
{
    return verb != NULL &&
           (lstrcmpiW(verb, L"copy-file") == 0 ||
            lstrcmpiW(verb, L"move-file") == 0 ||
            lstrcmpiW(verb, L"delete-file") == 0 ||
            lstrcmpiW(verb, L"create-dir") == 0 ||
            lstrcmpiW(verb, L"set-attributes") == 0 ||
            lstrcmpiW(verb, L"set-security") == 0);
}

static BOOL CanonicalizeArgumentPath(const wchar_t* path, wchar_t* out, DWORD outCount)
{
    if (path == NULL || *path == 0 || out == NULL || outCount == 0)
        return FALSE;

    DWORD len = GetFullPathNameW(path, outCount, out, NULL);
    if (len == 0 || len >= outCount)
        return FALSE;

    for (wchar_t* s = out; *s != 0; ++s)
    {
        if (*s == L'/')
            *s = L'\\';
    }
    return TRUE;
}

int wmain(int argc, wchar_t** argv)
{
    // Minimal broker skeleton: it is intentionally not a general command runner.
    // The main Salamander process must pass a fixed verb plus canonicalizable paths
    // over the documented protocol before operation execution is enabled here.
    if (argc < 3 || lstrcmpiW(argv[1], L"--protocol") != 0)
        return ERROR_INVALID_PARAMETER;

    int version = _wtoi(argv[2]);
    if (version != SALADMIN_PROTOCOL_VERSION)
        return ERROR_REVISION_MISMATCH;

    if (argc < 5 || lstrcmpiW(argv[3], L"--verb") != 0 || !IsAllowedVerb(argv[4]))
        return ERROR_INVALID_PARAMETER;

    for (int i = 5; i < argc; ++i)
    {
        if ((lstrcmpiW(argv[i], L"--source") == 0 || lstrcmpiW(argv[i], L"--target") == 0) && i + 1 < argc)
        {
            wchar_t canonical[MAX_PATH];
            if (!CanonicalizeArgumentPath(argv[++i], canonical, MAX_PATH))
                return ERROR_BAD_PATHNAME;
        }
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}
