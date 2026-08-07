// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <stddef.h>

// The first argument is normally GetACP(). A UTF-8 activeCodePage manifest
// makes the second argument authoritative for regional table auto-selection.
DWORD GetConversionAutoCodePage(DWORD activeCodePage, DWORD systemLocaleCodePage);
DWORD GetSystemLocaleAnsiCodePage();
// Code page shown by conversion-table UI and used for regional auto-selection.
DWORD GetEffectiveConversionCodePage();

// The parser keeps this separate so conversion names can be normalized only
// after the complete convert.cfg has supplied its metadata.
BOOL ParseConversionCodePageIdentifier(const char* text, size_t length, DWORD* identifier);

// Returned text uses the active process encoding expected by existing char*
// consumers and by the ANSI Win32 menu APIs. No public SDK signature changes.
BOOL ConvertConversionTableText(const char* source, UINT sourceCodePage,
                                UINT destinationCodePage, char* destination,
                                size_t destinationSize);
