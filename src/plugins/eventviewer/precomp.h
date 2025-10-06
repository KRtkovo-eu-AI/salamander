//****************************************************************************
//
// Copyright (c) ALTAP, spol. s r.o. All rights reserved.
//
// This is a part of the Altap Salamander SDK library.
//
// The SDK is provided "AS IS" and without warranty of any kind and
// ALTAP EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE and NON-INFRINGEMENT.
//
//****************************************************************************

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>
#include <winevt.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

#if defined(_DEBUG) && defined(_MSC_VER)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "versinfo.rh2"

#include "spl_com.h"
#include "spl_base.h"
#include "spl_gen.h"
#include "spl_menu.h"
#include "spl_view.h"
#include "spl_gui.h"
#include "spl_vers.h"

#include "dbg.h"
#include "mhandles.h"
#include "arraylt.h"
#include "winliblt.h"
#include "auxtools.h"

#include "plugin.h"
#include "plugin.rh"
#include "lang/lang.rh"

#ifdef __BORLANDC__
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif // __BORLANDC__
