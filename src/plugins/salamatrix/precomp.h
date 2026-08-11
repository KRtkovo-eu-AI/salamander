// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <CommDlg.h>
#include <shlobj.h>
#include <commctrl.h>
#include <ostream>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "../shared/spl_com.h"
#include "../shared/spl_base.h"
#include "../shared/spl_gen.h"
#include "../shared/spl_menu.h"
#include "../shared/spl_fs.h"
#include "../shared/spl_view.h"
#include "../shared/spl_thum.h"
#include "versinfo.rh2"
#include "../shared/spl_vers.h"
#include "../shared/spl_gui.h"
#include "../shared/dbg.h"
#include "../shared/mhandles.h"
#include "../../darkmode.h"

#include "salamatrix.h"
#include "salamatrix.rh"
#include "salamatrix_runtime.h"
#include "salamatrix_packages.h"
