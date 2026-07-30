// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <stdint.h>
#include <string>
#include <vector>

struct CDiskDirEntry
{
    std::string Path;
    uint64_t Size;
    FILETIME LastWrite;
    bool IsDirectory;
    bool HasLastWrite;
};

struct CDiskDirCatalog
{
    std::string SourceRoot;
    std::vector<CDiskDirEntry> Entries;
};

bool DiskDirReadCatalog(const char* fileName, CDiskDirCatalog& catalog, std::string& error);
bool DiskDirWriteAll(HANDLE file, const void* data, size_t size, std::string& error);
std::string DiskDirFormatEntry(const char* name, bool isDirectory, uint64_t size,
                               const FILETIME& lastWrite);
bool DiskDirNormalizeRelativePath(const std::string& input, std::string& output);
bool DiskDirResolveSourcePath(const CDiskDirCatalog& catalog, const std::string& relativePath,
                              std::string& sourcePath);
std::string DiskDirTextToUtf8(const char* text);
std::wstring DiskDirUtf8ToWide(const std::string& text);
std::wstring DiskDirPathToExtendedWide(const char* path);
bool DiskDirEnsureDirectory(const std::string& path);
