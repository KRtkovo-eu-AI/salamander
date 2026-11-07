// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

BOOL SplitDialog(const char* fileName,        // [in]     name of the file being split
                 CQuadWord& fileSize,    // [in]     its size
                 std::string& targetDir, // [in/out] target directory
                 const char* sourceDirectory,
                 CQuadWord* partialSize, // [out]    size of a single part
                 HWND hParent);

BOOL CombineDialog(TIndirectArray<char>& files, // [in/out] array of partial file names
                   std::string& targetName,     // [in/out] name of the target file
                   BOOL bOrigCrcFound,          // [in]     flag indicating whether the original CRC was found
                   UINT32 origCrc,              // [in]     original CRC
                   HWND hParent, CSalamanderForOperationsAbstract* salamander);

void ConfigDialog(HWND hParent);

void CRCDialog(TIndirectArray<char>& files, BOOL bCrcFound, UINT32 origCrc,
               HWND parent, CSalamanderForOperationsAbstract* salamander);
