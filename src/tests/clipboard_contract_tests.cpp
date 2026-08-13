// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../clipboard_data_object_contract.h"

#include <iostream>

namespace
{
int Fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
}
}

int main()
{
    const CLIPFORMAT privateFormat = 0xC123;
    FORMATETC format = {privateFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (QueryPrivateHGlobalClipboardFormat(&format, privateFormat) != S_OK)
        return Fail("The private HGLOBAL format must be queryable without rendering data.");
    if (QueryPrivateHGlobalClipboardFormat(NULL, privateFormat) != E_INVALIDARG)
        return Fail("A null FORMATETC must be rejected.");

    format.cfFormat++;
    if (QueryPrivateHGlobalClipboardFormat(&format, privateFormat) != DV_E_FORMATETC)
        return Fail("A different clipboard format must be rejected.");
    format.cfFormat = privateFormat;
    format.dwAspect = DVASPECT_THUMBNAIL;
    if (QueryPrivateHGlobalClipboardFormat(&format, privateFormat) != DV_E_DVASPECT)
        return Fail("A non-content aspect must be rejected.");
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex = 0;
    if (QueryPrivateHGlobalClipboardFormat(&format, privateFormat) != DV_E_LINDEX)
        return Fail("An indexed request must be rejected.");
    format.lindex = -1;
    format.tymed = TYMED_ISTREAM;
    if (QueryPrivateHGlobalClipboardFormat(&format, privateFormat) != DV_E_TYMED)
        return Fail("A request without TYMED_HGLOBAL must be rejected.");

    std::cout << "Clipboard data-object contract tests passed.\n";
    return 0;
}
