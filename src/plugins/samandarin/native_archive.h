// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Define safe native 7z package extraction. | Remove the managed SharpCompress dependency. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include "native_catalog.h"

namespace SamandarinNative
{
bool ExtractAndValidatePackage(const std::wstring& archivePath, const std::wstring& payloadDirectory,
                               PackageKind kind, std::wstring* extractedRoot,
                               std::wstring* error);
} // namespace SamandarinNative
