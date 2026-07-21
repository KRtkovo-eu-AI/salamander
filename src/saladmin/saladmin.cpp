// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include <stdlib.h>
#include <aclapi.h>

#define SALADMIN_PROTOCOL_VERSION 1

struct CBrokerRequest
{
    const wchar_t* Verb;
    wchar_t Source[MAX_PATH];
    wchar_t Target[MAX_PATH];
    DWORD Attributes;
    BOOL HaveSource;
    BOOL HaveTarget;
    BOOL HaveAttributes;
};

static BOOL IsAllowedVerb(const wchar_t* verb)
{
    return verb != NULL &&
           (lstrcmpiW(verb, L"copy-file") == 0 ||
            lstrcmpiW(verb, L"move-file") == 0 ||
            lstrcmpiW(verb, L"delete-file") == 0 ||
            lstrcmpiW(verb, L"create-dir") == 0 ||
            lstrcmpiW(verb, L"set-attributes") == 0 ||
            lstrcmpiW(verb, L"copy-security") == 0);
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

static BOOL ParseDword(const wchar_t* text, DWORD* value)
{
    if (text == NULL || value == NULL)
        return FALSE;

    wchar_t* end = NULL;
    unsigned long parsed = wcstoul(text, &end, 0);
    if (end == text || *end != 0)
        return FALSE;

    *value = (DWORD)parsed;
    return TRUE;
}

static DWORD ParseRequest(int argc, wchar_t** argv, CBrokerRequest* request)
{
    if (request == NULL)
        return ERROR_INVALID_PARAMETER;

    ZeroMemory(request, sizeof(*request));
    request->Attributes = INVALID_FILE_ATTRIBUTES;

    if (argc < 5 || lstrcmpiW(argv[1], L"--protocol") != 0)
        return ERROR_INVALID_PARAMETER;

    int version = _wtoi(argv[2]);
    if (version != SALADMIN_PROTOCOL_VERSION)
        return ERROR_REVISION_MISMATCH;

    if (lstrcmpiW(argv[3], L"--verb") != 0 || !IsAllowedVerb(argv[4]))
        return ERROR_INVALID_PARAMETER;
    request->Verb = argv[4];

    for (int i = 5; i < argc; ++i)
    {
        if (lstrcmpiW(argv[i], L"--source") == 0 && i + 1 < argc)
        {
            if (!CanonicalizeArgumentPath(argv[++i], request->Source, MAX_PATH))
                return ERROR_BAD_PATHNAME;
            request->HaveSource = TRUE;
        }
        else if (lstrcmpiW(argv[i], L"--target") == 0 && i + 1 < argc)
        {
            if (!CanonicalizeArgumentPath(argv[++i], request->Target, MAX_PATH))
                return ERROR_BAD_PATHNAME;
            request->HaveTarget = TRUE;
        }
        else if (lstrcmpiW(argv[i], L"--attributes") == 0 && i + 1 < argc)
        {
            if (!ParseDword(argv[++i], &request->Attributes))
                return ERROR_INVALID_PARAMETER;
            request->HaveAttributes = TRUE;
        }
        else
            return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

static DWORD ExecuteRequest(const CBrokerRequest* request)
{
    if (request == NULL || request->Verb == NULL)
        return ERROR_INVALID_PARAMETER;

    if (lstrcmpiW(request->Verb, L"copy-file") == 0)
    {
        if (!request->HaveSource || !request->HaveTarget)
            return ERROR_INVALID_PARAMETER;
        return CopyFileW(request->Source, request->Target, FALSE) ? ERROR_SUCCESS : GetLastError();
    }
    if (lstrcmpiW(request->Verb, L"move-file") == 0)
    {
        if (!request->HaveSource || !request->HaveTarget)
            return ERROR_INVALID_PARAMETER;
        return MoveFileExW(request->Source, request->Target, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) ? ERROR_SUCCESS : GetLastError();
    }
    if (lstrcmpiW(request->Verb, L"delete-file") == 0)
    {
        if (!request->HaveSource)
            return ERROR_INVALID_PARAMETER;
        DWORD attrs = GetFileAttributesW(request->Source);
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
            return RemoveDirectoryW(request->Source) ? ERROR_SUCCESS : GetLastError();
        return DeleteFileW(request->Source) ? ERROR_SUCCESS : GetLastError();
    }
    if (lstrcmpiW(request->Verb, L"create-dir") == 0)
    {
        if (!request->HaveTarget)
            return ERROR_INVALID_PARAMETER;
        return CreateDirectoryW(request->Target, NULL) || GetLastError() == ERROR_ALREADY_EXISTS ? ERROR_SUCCESS : GetLastError();
    }
    if (lstrcmpiW(request->Verb, L"set-attributes") == 0)
    {
        if (!request->HaveSource || !request->HaveAttributes)
            return ERROR_INVALID_PARAMETER;
        return SetFileAttributesW(request->Source, request->Attributes) ? ERROR_SUCCESS : GetLastError();
    }
    if (lstrcmpiW(request->Verb, L"copy-security") == 0)
    {
        if (!request->HaveSource || !request->HaveTarget)
            return ERROR_INVALID_PARAMETER;

        PSECURITY_DESCRIPTOR sd = NULL;
        PSID owner = NULL;
        PSID group = NULL;
        PACL dacl = NULL;
        DWORD err = GetNamedSecurityInfoW(request->Source, SE_FILE_OBJECT,
                                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                          &owner, &group, &dacl, NULL, &sd);
        if (err == ERROR_SUCCESS)
        {
            err = SetNamedSecurityInfoW(request->Target, SE_FILE_OBJECT,
                                        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                        owner, group, dacl, NULL);
        }
        if (sd != NULL)
            LocalFree(sd);
        return err;
    }

    return ERROR_INVALID_PARAMETER;
}

int wmain(int argc, wchar_t** argv)
{
    CBrokerRequest request;
    DWORD err = ParseRequest(argc, argv, &request);
    if (err != ERROR_SUCCESS)
        return (int)err;

    return (int)ExecuteRequest(&request);
}
