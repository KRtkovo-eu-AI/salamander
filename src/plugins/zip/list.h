// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// | 2026-11-08 | Declare the protected ZIP listing entry point. | Reuse the central directory reader. | | GPT-6.1 Sol (Codex) |
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class CZipList : public CZipCommon
{
public:
    CZipList(const char* zipName, CSalamanderForOperationsAbstract* salamander) : CZipCommon(zipName, "", salamander, NULL)
    {
        Extract = true;
    }
    DWORD ListSource(const SalZipReadSourceV1& source);
    int ListArchive(CSalamanderDirectoryAbstract* dir, BOOL& haveFiles);
    int List(CSalamanderDirectoryAbstract* dir, BOOL& haveFiles);
};
