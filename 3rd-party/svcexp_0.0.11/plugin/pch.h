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

#include <windows.h>
#include <shlobj.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif // _MSC_VER
#include <limits.h>
#include <process.h>
#include <commctrl.h>
#include <ostream>
#include <stdio.h>
#include <time.h>

#include "versinfo.rh"

#include "spl_com.h"
#include "spl_arc.h"
#include "spl_base.h"
#include "spl_gen.h"
#include "spl_fs.h"
#include "spl_menu.h"
#include "spl_thum.h"
#include "spl_view.h"
#include "spl_vers.h"
#include "spl_gui.h"

#include "dbg.h"
#include "mhandles.h"
#include "arraylt.h"
#include "winliblt.h"
#include "auxtools.h"

#include "servicemanager.h"
#include "plugin.h"
#include "dialogs.h"
#include "plugin.rh"
#include "lang.rh"

#ifdef __BORLANDC__
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif // __BORLANDC__
