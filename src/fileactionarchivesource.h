// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Internal host identity. Include after fileactiondisktarget.h, fileswnd.h,
// mainwnd.h and plugins.h. This does not own or clone plugin-private entry data:
// callers must validate before borrowing the listing and at callback boundaries.
struct CFilesActionArchiveSource
{
    CFilesWindow* Panel = NULL;
    ULONGLONG Tab = 0, ListingGeneration = 0;
    int VisibleSide = 0;
    std::string ArchiveFile, ArchiveRoot, Directory;
    CSalamanderDirectory* ArchiveDirectory = NULL;
    CFilesArray* Files = NULL;
    CFilesArray* Dirs = NULL;
    CPluginDataInterfaceAbstract* PluginData = NULL;
    CPluginData* Owner = NULL;

    bool Capture(CMainWindow* main, CFilesWindow* panel)
    try
    {
        Panel = NULL;
        CFilesWindow* live = CFilesActionDiskTarget::FindLive(main, panel);
        if (live == NULL || !live->Is(ptZIPArchive) || live->IsBranchView() ||
            live->GetPanelTabId() == 0 || live->GetArchiveDir() == NULL ||
            live->Files == NULL || live->Dirs == NULL || live->GetZIPArchive()[0] == 0)
            return false;
        // Copy all path text before the first modal/callback. A/B/A relisting
        // remains stale even when allocator addresses and path strings match.
        ArchiveFile = live->GetZIPArchive();
        ArchiveRoot = live->GetZIPPath();
        Directory = live->GetPath();
        Tab = live->GetPanelTabId();
        ListingGeneration = live->GetListingIdentityGeneration();
        ArchiveDirectory = live->GetArchiveDir();
        Files = live->Files;
        Dirs = live->Dirs;
        PluginData = live->PluginData.GetInterface();
        Owner = live->GetPluginDataForPluginIface();
        VisibleSide = main->LeftPanel == live ? 1 : main->RightPanel == live ? 2 : 0;
        if (PluginData != NULL && Owner == NULL) return false;
        Panel = live;
        return true;
    }
    catch (const std::bad_alloc&)
    {
        Panel = NULL;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return false;
    }

    bool IsCurrent(CMainWindow* main) const
    {
        CFilesWindow* live = CFilesActionDiskTarget::FindLive(main, Panel);
        // Never dereference a saved panel/listing/interface before membership,
        // tab and epoch validation; the old addresses may already be retired.
        return live != NULL && live->GetPanelTabId() == Tab &&
            live->GetListingIdentityGeneration() == ListingGeneration &&
            live->Is(ptZIPArchive) && !live->IsBranchView() &&
            (VisibleSide != 1 || main->LeftPanel == live) &&
            (VisibleSide != 2 || main->RightPanel == live) &&
            ArchiveFile == live->GetZIPArchive() && ArchiveRoot == live->GetZIPPath() &&
            Directory == live->GetPath() && ArchiveDirectory == live->GetArchiveDir() &&
            Files == live->Files && Dirs == live->Dirs &&
            PluginData == live->PluginData.GetInterface() &&
            Owner == live->GetPluginDataForPluginIface();
    }
};
