// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Define native hashing and Authenticode package verification. | Replace managed SHA-256 and WinVerifyTrust wrappers. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include "native_catalog.h"

namespace SamandarinNative
{
bool ComputeSha256(const std::wstring& path, std::wstring* hash, std::wstring* error);
bool VerifyExtractedPackage(const std::wstring& root, PackageKind kind,
                            std::wstring* firstPartyPublisher, std::wstring* error);
} // namespace SamandarinNative
