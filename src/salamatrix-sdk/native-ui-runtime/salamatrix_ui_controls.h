// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>
#include "../../plugins/shared/spl_com.h"
#include "../../plugins/shared/spl_gui.h"

namespace Salamatrix { namespace UI {

CGUIStaticTextAbstract* AttachNativeStaticText(HWND parent, int id, DWORD flags);
CGUIHyperLinkAbstract* AttachNativeHyperLink(HWND parent, int id, DWORD flags);
CGUIProgressBarAbstract* AttachNativeProgressBar(HWND parent, int id);
BOOL ChangeNativeArrowButton(HWND parent, int id);
CGUIButtonAbstract* AttachNativeButton(HWND parent, int id, DWORD flags);
CGUIColorArrowButtonAbstract* AttachNativeColorArrowButton(HWND parent, int id, BOOL showArrow);
CGUIToolbarHeaderAbstract* AttachNativeToolbarHeader(HWND parent, int id, HWND alignWindow, DWORD mask);

} }
