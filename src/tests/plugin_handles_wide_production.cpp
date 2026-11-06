// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Compile the entire production plugin tracker unchanged. Only diagnostic
// allocation/conversion errors are injected; registry and Win32 I/O stay real.
#include "plugin_handles_wide_test_support/precomp.h"
#ifdef MHANDLES_ENABLE
static bool DiagnosticArmed = false;
static DiagnosticFault Fault = DiagnosticNormal;
static int Allocations = 0, Frees = 0, Conversions = 0;
static std::string Captured;
void ArmDiagnostic(DiagnosticFault mode) { Fault=mode; Allocations=Frees=Conversions=0; Captured.clear(); DiagnosticArmed=true; }
void DisarmDiagnostic() { DiagnosticArmed=false; }
int DiagnosticAllocations() { return Allocations; }
int DiagnosticFrees() { return Frees; }
int DiagnosticConversions() { return Conversions; }
const char* CapturedDiagnostic() { return Captured.c_str(); }
static void* DiagnosticMalloc(size_t bytes)
{
    if (DiagnosticArmed && Fault == DiagnosticNoMemory) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return nullptr; }
    void* result = std::malloc(bytes);
    if (DiagnosticArmed && result) ++Allocations;
    return result;
}
static void DiagnosticFree(void* pointer)
{
    if (DiagnosticArmed && pointer) ++Frees;
    std::free(pointer);
    if (DiagnosticArmed) SetLastError(ERROR_WRITE_FAULT);
}
static int DiagnosticWideCharToMultiByte(UINT page, DWORD flags, LPCWCH source, int sourceCount,
    LPSTR output, int outputCount, LPCCH defaultChar, LPBOOL usedDefault)
{
    if (DiagnosticArmed)
    {
        ++Conversions;
        if (Fault == DiagnosticSizeFailure || (Fault == DiagnosticConvertFailure && output))
        { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return 0; }
    }
    const int result = ::WideCharToMultiByte(page,flags,source,sourceCount,output,outputCount,defaultChar,usedDefault);
    if (DiagnosticArmed && result > 0 && output) Captured.assign(output);
    if (DiagnosticArmed) SetLastError(ERROR_BAD_ENVIRONMENT);
    return result;
}
#define malloc DiagnosticMalloc
#define free DiagnosticFree
#define WideCharToMultiByte DiagnosticWideCharToMultiByte
#endif
#ifndef PLUGIN_HANDLES_SOURCE
#define PLUGIN_HANDLES_SOURCE "../plugins/shared/mhandles.cpp"
#endif
#include PLUGIN_HANDLES_SOURCE
#ifdef MHANDLES_ENABLE
#undef malloc
#undef free
#undef WideCharToMultiByte
#ifdef new
#undef new
#endif
char* TestNullDiagnostic() { return HandlesWidePathDiagnostic(nullptr); }
#endif
