// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Internal host implementation. Include after mainwnd.h, fileswnd.h and common/widepath.h.
// Shared by disk file actions and archive destination lifecycle validation.
// S31b5b2: own the disk destination across modal dispatch. A saved pointer is
// compared only with live tab registry entries before it is dereferenced.
struct CFilesActionDiskTarget
{
    CFilesWindow* Panel = NULL;
    ULONGLONG Tab = 0, Generation = 0;
    std::wstring Root;
    std::string DefaultPath;
    BOOL Branch = FALSE;
    BOOL DirectoryDefault = FALSE;
    int VisibleSide = 0;

    static bool AllowsBranchTransfer(CActionType type, int countSizeMode, BOOL selectedDirs,
                                 BOOL sourceDisk, BOOL targetDisk)
    {
        return (type == atCopy || type == atMove) && countSizeMode == 0 && !selectedDirs && sourceDisk && targetDisk;
    }

    static CFilesWindow* FindLive(CMainWindow* main, CFilesWindow* expected)
    {
        if (main == NULL || expected == NULL) return NULL;
        const CPanelSide sides[] = {cpsLeft, cpsRight};
        for (int side = 0; side < 2; ++side)
            for (int index = 0; index < main->GetPanelTabCount(sides[side]); ++index)
            {
                CFilesWindow* current = main->GetPanelTabAt(sides[side], index);
                if (current == expected) return current;
            }
        for (int index = 0; index < main->GetDetachedTabCount(); ++index)
        {
            CFilesWindow* current = main->GetDetachedTabAt(index);
            if (current == expected) return current;
        }
        return NULL;
    }

    bool Capture(CMainWindow* main, CFilesWindow* panel, BOOL directoryDefault = FALSE)
    {
        Panel = NULL;
        CFilesWindow* live = FindLive(main, panel);
        if (live == NULL || !live->Is(ptDisk) || live->IsBranchViewScanning()) return false;
        const std::wstring root(live->GetPathW());
        const std::string encoded = SalWideToMultiBytePath(root.c_str(), CP_UTF8);
        if (root.empty() || SalMultiByteToWidePath(encoded.c_str(), CP_UTF8) != root) return false;
        const std::string defaultPath = directoryDefault ? encoded : encoded + (root.back() == L'\\' ? "*.*" : "\\*.*");
        if (defaultPath.size() >= SAL_MAX_PATH || live->GetPanelTabId() == 0) return false;
        Root = root;
        DefaultPath = defaultPath;
        DirectoryDefault = directoryDefault;
        Tab = live->GetPanelTabId();
        Branch = live->IsBranchView();
        Generation = Branch ? live->GetBranchViewGeneration() : 0;
        VisibleSide = main->LeftPanel == live ? 1 : main->RightPanel == live ? 2 : 0;
        Panel = live;
        return true;
    }

    // Classify the unmodified dialog result. A manually entered destination has
    // its own resolver semantics; it is not an implicit panel/drop root.
    bool HasDefaultText(const char* path, int capacity) const
    {
        return Panel != NULL && !Root.empty() && !DefaultPath.empty() && path != NULL &&
               capacity > 0 && DefaultPath.size() < (size_t)(std::min)(capacity, SAL_MAX_PATH) &&
               memcmp(path, DefaultPath.c_str(), DefaultPath.size() + 1) == 0;
    }

    bool MatchesParsedDefault(const char* path, int capacity, const char* secondPart,
                              int pathType, BOOL pathIsDir) const
    {
        // An absent root would leave additional components after secondPart and
        // let the split/create helper recreate it. Refuse before that helper.
        return !DirectoryDefault && pathType == PATH_TYPE_WINDOWS && pathIsDir && HasDefaultText(path, capacity) &&
               DefaultPath.size() >= 3 && secondPart == path + DefaultPath.size() - 3;
    }

    // Archive extraction accepts an existing directory, never an operation mask.
    bool MatchesParsedDirectoryDefault(const char* path, int capacity, const char* secondPart,
                                       int pathType, BOOL pathIsDir) const
    {
        return DirectoryDefault && pathType == PATH_TYPE_WINDOWS && pathIsDir &&
               HasDefaultText(path, capacity) && secondPart == path + DefaultPath.size();
    }

    bool MatchesSplitDefault(const char* path, int capacity, const char* mask) const
    {
        if (DirectoryDefault || Panel == NULL || Root.empty() || DefaultPath.size() < 3 || path == NULL || capacity <= 0)
            return false;
        const size_t limit = (size_t)(std::min)(capacity, SAL_MAX_PATH);
        const char* end = (const char*)memchr(path, 0, limit);
        if (end == NULL) return false;
        const size_t directoryLength = (size_t)(end - path);
        // Only inspect the mask after checking that it belongs to caller storage
        // immediately after the directory NUL and fits together with its own NUL.
        if (mask != end + 1 || limit - directoryLength - 1 < 4 ||
            memcmp(mask, "*.*", 4) != 0) return false;
        const size_t prefixLength = DefaultPath.size() - 3; // includes root separator
        const bool driveRoot = prefixLength == 3 && DefaultPath[1] == ':' && DefaultPath[2] == '\\';
        if (directoryLength != prefixLength &&
            (driveRoot || directoryLength + 1 != prefixLength)) return false;
        return memcmp(path, DefaultPath.data(), directoryLength) == 0;
    }

    bool IsCurrent(CMainWindow* main) const
    {
        CFilesWindow* live = FindLive(main, Panel);
        return live != NULL && live->GetPanelTabId() == Tab && live->Is(ptDisk) &&
               !live->IsBranchViewScanning() && Root == live->GetPathW() &&
               live->IsBranchView() == Branch &&
               (!Branch || Generation == live->GetBranchViewGeneration()) &&
               (VisibleSide != 1 || main->LeftPanel == live) &&
               (VisibleSide != 2 || main->RightPanel == live);
    }
};
