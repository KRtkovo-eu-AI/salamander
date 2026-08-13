// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <objidl.h>

inline HRESULT QueryPrivateHGlobalClipboardFormat(const FORMATETC* formatEtc, CLIPFORMAT clipboardFormat)
{
    if (formatEtc == NULL)
        return E_INVALIDARG;
    if (formatEtc->cfFormat != clipboardFormat)
        return DV_E_FORMATETC;
    if (formatEtc->dwAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;
    if (formatEtc->lindex != -1)
        return DV_E_LINDEX;
    if ((formatEtc->tymed & TYMED_HGLOBAL) == 0)
        return DV_E_TYMED;
    return S_OK;
}
