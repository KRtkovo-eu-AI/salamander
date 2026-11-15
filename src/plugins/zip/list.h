// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-15 | Explain protected ZIP listing entry points. | Keep scoped range reads on the ordinary listing parser. | #731 | GPT-6 |
// | 2026-11-08 | Use the common verified range-source initializer. | Keep archive panel parsing on the shared source. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Expose the verified range source to the normal ZIP listing. | Reuse regular archive entries. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Declare the protected ZIP listing entry point. | Reuse the central directory reader. | | GPT-6.1 Sol (Codex) |

#pragma once

class CZipList : public CZipCommon
{
public:
    CZipList(const char* zipName, CSalamanderForOperationsAbstract* salamander) : CZipCommon(zipName, "", salamander, NULL)
    {
        Extract = true;
    }
    // Protected archives use the same central-directory parser through a scoped source.
    DWORD ListSource(const SalZipReadSourceV1& source);
    int ListArchive(CSalamanderDirectoryAbstract* dir, BOOL& haveFiles);
    int List(CSalamanderDirectoryAbstract* dir, BOOL& haveFiles, CPluginDataInterfaceAbstract* pluginData = nullptr);
};
