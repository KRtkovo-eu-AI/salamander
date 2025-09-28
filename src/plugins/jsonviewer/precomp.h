// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include <algorithm>

#include "versinfo.rh2"

#include "spl_com.h"
#include "spl_base.h"
#include "spl_gen.h"
#include "spl_view.h"
#include "spl_vers.h"
#include "spl_gui.h"

#include "dbg.h"
#include "mhandles.h"

#include "jsonviewer.rh"
#include "jsonviewer.rh2"

#ifdef __BORLANDC__
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif // __BORLANDC__
