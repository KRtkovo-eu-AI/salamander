// SPDX-License-Identifier: GPL-2.0-or-later
// | 2026-11-08 | Share keyword extraction and matching with scoped Find. | Keep metadata handling in the application. | | GPT-6.1 Sol (Codex) |

#pragma once

#include <string>
#include <vector>
#include <propkey.h>

struct IStream;

enum CFileTagsOperation
{
    ftoAdd,
    ftoRemove,
    ftoReplace
};

enum CFileTagsMatchMode
{
    ftmmAny,
    ftmmAll,
    ftmmNone
};

void ParseFileTagsW(const wchar_t* text, std::vector<std::wstring>& tags);
std::wstring FormatFileTagsW(const std::vector<std::wstring>& tags);
HRESULT ReadFileTagsW(const wchar_t* path, std::vector<std::wstring>& tags);
HRESULT ReadFileTagsFromStreamW(const wchar_t* path, IStream* stream, std::vector<std::wstring>& tags);
BOOL FileTagValuesMatchW(const std::vector<std::wstring>& actual, const std::vector<std::wstring>& requested,
                         CFileTagsMatchMode mode);
BOOL IsFilePropertyWritableW(const wchar_t* path, REFPROPERTYKEY key);
HRESULT WriteFileTagsW(const wchar_t* path, const std::vector<std::wstring>& tags);
HRESULT WriteFileStringVectorPropertyW(const wchar_t* path, REFPROPERTYKEY key,
                                       const std::vector<std::wstring>& values);
HRESULT UpdateFileTagsW(const wchar_t* path, const std::vector<std::wstring>& tags,
                        CFileTagsOperation operation);
BOOL FileTagsMatchW(const wchar_t* path, const std::vector<std::wstring>& tags,
                    CFileTagsMatchMode mode);

HRESULT ReadFilePropertyTextW(const wchar_t* path, REFPROPERTYKEY key, std::wstring& value);
HRESULT WriteFilePropertyTextW(const wchar_t* path, REFPROPERTYKEY key,
                               const wchar_t* value, BOOL clearValue);
