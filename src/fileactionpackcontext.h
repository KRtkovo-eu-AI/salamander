// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Host-only binding for qualified disk Pack operations. Include after zip.h,
// plugins.h, fileswnd.h, mainwnd.h and fileactiondisktarget.h.
struct CFilesActionPackContext
{
    CFilesActionDiskTarget Source, Target;
    ULONGLONG ListingGeneration = 0;
    CFilesArray* Files = NULL;
    CFilesArray* Dirs = NULL;
    int PackerIndex = -1, PackerType = 0;
    CPluginData* Backend = NULL;
    std::string BackendName;

    bool Capture(CMainWindow* main, CFilesWindow* source, CFilesWindow* target)
    try
    {
        Backend = NULL;
        if (!Source.Capture(main, source, TRUE) || !Target.Capture(main, target, TRUE)) return false;
        ListingGeneration = source->GetListingIdentityGeneration();
        Files = source->Files;
        Dirs = source->Dirs;
        return Files != NULL && Dirs != NULL;
    }
    catch (const std::bad_alloc&)
    { Source.Panel = NULL; Target.Panel = NULL; SetLastError(ERROR_NOT_ENOUGH_MEMORY); return false; }

    bool PanelsAreCurrent(CMainWindow* main) const
    {
        // Validate registry/tab/root before dereferencing saved source arrays.
        return Source.IsCurrent(main) && Target.IsCurrent(main) &&
            Source.Panel->GetListingIdentityGeneration() == ListingGeneration &&
            Source.Panel->Files == Files && Source.Panel->Dirs == Dirs;
    }

    bool BindBackend(CMainWindow* main)
    try
    {
        Backend = NULL;
        if (!PanelsAreCurrent(main)) { SetLastError(ERROR_RETRY); return false; }
        const int index = PackerConfig.GetPreferedPacker();
        if (index < 0 || index >= PackerConfig.GetPackersCount()) { SetLastError(ERROR_NOT_SUPPORTED); return false; }
        const int type = PackerConfig.GetPackerType(index);
        CPluginData* plugin = type < 0 ? Plugins.Get(-(type + 1)) : NULL;
        if (plugin == NULL || !plugin->SupportCustomPack || plugin->DLLName == NULL ||
            (_stricmp(plugin->DLLName, "zip\\zip.spl") != 0 &&
             _stricmp(plugin->DLLName, "7zip\\7zip.spl") != 0))
        { SetLastError(ERROR_NOT_SUPPORTED); return false; }
        BackendName = plugin->DLLName;
        PackerIndex = index;
        PackerType = type;
        Backend = plugin;
        return true;
    }
    catch (const std::bad_alloc&)
    { Backend = NULL; SetLastError(ERROR_NOT_ENOUGH_MEMORY); return false; }

    bool IsCurrent(CMainWindow* main) const
    {
        return PanelsAreCurrent(main) && (Backend == NULL ||
            (PackerIndex == PackerConfig.GetPreferedPacker() &&
             PackerIndex >= 0 && PackerIndex < PackerConfig.GetPackersCount() &&
             PackerConfig.GetPackerType(PackerIndex) == PackerType &&
             Plugins.Get(-(PackerType + 1)) == Backend && Backend->SupportCustomPack &&
             Backend->DLLName != NULL && BackendName == Backend->DLLName));
    }
};
