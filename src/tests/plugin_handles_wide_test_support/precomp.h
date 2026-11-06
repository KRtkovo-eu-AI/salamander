// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <oleidl.h>
#include <commctrl.h>
#include <process.h>
#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <ostream>
#include <sstream>
#include <algorithm>
#include <string>
using std::max;
#include "spl_base.h"
// Legacy SDK debug stubs have unused arguments under native tests' W4.
#pragma warning(push)
#pragma warning(disable: 4100)
#include "dbg.h"
#pragma warning(pop)
#include "mhandles.h"
enum DiagnosticFault { DiagnosticNormal, DiagnosticNoMemory, DiagnosticSizeFailure, DiagnosticConvertFailure };
void ArmDiagnostic(DiagnosticFault mode);
void DisarmDiagnostic();
int DiagnosticAllocations();
int DiagnosticFrees();
int DiagnosticConversions();
const char* CapturedDiagnostic();
char* TestNullDiagnostic();
