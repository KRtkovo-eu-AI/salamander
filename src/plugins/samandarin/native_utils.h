// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Add Unicode and long-path helpers for native updater files. | Keep catalog and installer paths wide end to end. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include <string>

namespace SamandarinNative
{
std::wstring Utf8ToWide(const std::string& text);
std::wstring AnsiOrUtf8ToWide(const std::string& text);
std::string WideToUtf8(const std::wstring& text);
std::string WideToAnsi(const std::wstring& text);
std::wstring GetModulePath(HINSTANCE module);
std::wstring GetModuleDirectory(HINSTANCE module);
std::wstring GetExecutableDirectory();
std::wstring AddLongPathPrefix(const std::wstring& path);
std::wstring FullPath(const std::wstring& path);
std::wstring CombinePath(const std::wstring& directory, const std::wstring& child);
std::wstring GetParentDirectory(const std::wstring& path);
std::wstring GetFileName(const std::wstring& path);
std::wstring GetFileNameWithoutExtension(const std::wstring& path);
std::wstring GetFileExtension(const std::wstring& path);
bool IsPathInside(const std::wstring& path, const std::wstring& root);
bool PathExists(const std::wstring& path, bool* isDirectory = nullptr);
bool CreateDirectories(const std::wstring& path);
bool ReadUtf8File(const std::wstring& path, std::string* contents, size_t maximumBytes);
bool WriteUtf8File(const std::wstring& path, const std::string& contents, bool createNew = false);
bool WriteUtf8FileAtomic(const std::wstring& path, const std::string& contents);
} // namespace SamandarinNative
