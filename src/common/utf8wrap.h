#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI SalExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* rect, LPCSTR str, UINT count, const INT* dx);
BOOL WINAPI SalTextOutA(HDC hdc, int x, int y, LPCSTR str, int count);
BOOL WINAPI SalGetTextExtentPoint32A(HDC hdc, LPCSTR str, int count, LPSIZE size);
BOOL WINAPI SalGetTextExtentExPointA(HDC hdc, LPCSTR str, int count, int maxExtent, LPINT fit, LPINT dx, LPSIZE size);
int WINAPI SalDrawTextA(HDC hdc, LPCSTR str, int count, LPRECT rect, UINT format);
int WINAPI SalDrawTextExA(HDC hdc, LPTSTR str, int count, LPRECT rect, UINT format, LPDRAWTEXTPARAMS params);

#ifdef __cplusplus
}
#endif

#ifndef UTF8WRAPPERS_IMPLEMENTATION

#define ExtTextOutA SalExtTextOutA
#undef ExtTextOut
#define ExtTextOut SalExtTextOutA

#define TextOutA SalTextOutA
#undef TextOut
#define TextOut SalTextOutA

#define GetTextExtentPoint32A SalGetTextExtentPoint32A
#undef GetTextExtentPoint32
#define GetTextExtentPoint32 SalGetTextExtentPoint32A

#define GetTextExtentExPointA SalGetTextExtentExPointA
#undef GetTextExtentExPoint
#define GetTextExtentExPoint SalGetTextExtentExPointA

#define DrawTextA SalDrawTextA
#undef DrawText
#define DrawText SalDrawTextA

#define DrawTextExA SalDrawTextExA
#undef DrawTextEx
#define DrawTextEx SalDrawTextExA

#endif // UTF8WRAPPERS_IMPLEMENTATION

