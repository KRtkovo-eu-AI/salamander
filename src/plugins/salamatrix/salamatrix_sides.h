// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_sides.h
    Runtime-neutral side and panel-tab snapshot service.
*/

#pragma once

#include <strsafe.h>

#include "../shared/spl_gen.h"

namespace Salamatrix
{
    namespace Sides
    {

#define SALAMATRIX_SERVICE_SIDES "Salamatrix.Sides"
#define SALAMATRIX_SIDES_VERSION_1_0 0x00010000
#define SALAMATRIX_SIDE_ITEM_NAME_CAPACITY 512
#define SALAMATRIX_SIDE_ITEM_PATH_CAPACITY 32768

        enum SideReference
        {
            SideReferenceLeft = 1,
            SideReferenceRight = 2,
            SideReferenceSource = 3,
            SideReferenceTarget = 4
        };

        enum TabFlags
        {
            TabFlagNone = 0x00000000,
            TabFlagActiveOnSide = 0x00000001,
            TabFlagSource = 0x00000002,
            TabFlagTarget = 0x00000004,
            TabFlagLocked = 0x00000008,
            TabFlagDetached = 0x00000010
        };

        struct TabInfo
        {
            DWORD StructSize;
            ULONGLONG TabId;
            SideReference PhysicalSide;
            int Index;
            int PathType;
            DWORD Flags;

            TabInfo()
                : StructSize(sizeof(TabInfo)),
                  TabId(0),
                  PhysicalSide(SideReferenceLeft),
                  Index(-1),
                  PathType(0),
                  Flags(TabFlagNone)
            {
            }
        };

        struct ItemInfo
        {
            DWORD StructSize;
            char Name[SALAMATRIX_SIDE_ITEM_NAME_CAPACITY];
            char Path[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
            CQuadWord Size;
            DWORD Attributes;
            BOOL IsDirectory;

            ItemInfo()
                : StructSize(sizeof(ItemInfo)),
                  Size(0, 0),
                  Attributes(0),
                  IsDirectory(FALSE)
            {
                Name[0] = '\0';
                Path[0] = '\0';
            }
        };

        class ISidesService
        {
        public:
            virtual DWORD WINAPI GetVersion() const = 0;
            virtual SideReference WINAPI ResolveSide(SideReference side) const = 0;
            virtual int WINAPI GetTabCount(SideReference side) const = 0;
            virtual BOOL WINAPI GetTabInfo(SideReference side, int index, TabInfo* info) const = 0;
            virtual BOOL WINAPI GetTabInfoById(ULONGLONG tabId, TabInfo* info) const = 0;
            virtual BOOL WINAPI GetActiveTabInfo(SideReference side, TabInfo* info) const = 0;
            virtual BOOL WINAPI GetTabPath(
                ULONGLONG tabId,
                char* buffer,
                int bufferSize,
                int* pathType) const = 0;
            virtual BOOL WINAPI ActivateTab(ULONGLONG tabId, BOOL focus) = 0;
            virtual BOOL WINAPI ChangeActiveTabPath(
                SideReference side,
                const char* path,
                int* failReason) = 0;
            virtual BOOL WINAPI GetPath(
                SideReference side,
                char* buffer,
                int bufferSize,
                int* pathType) const = 0;
            virtual int WINAPI GetSelectedItemCount(SideReference side) const = 0;
            virtual BOOL WINAPI GetSelectedItem(
                SideReference side,
                int index,
                ItemInfo* info) const = 0;
            virtual BOOL WINAPI GetFocusedItem(
                SideReference side,
                ItemInfo* info) const = 0;

        protected:
            virtual ~ISidesService() {}
        };

        class SidesService : public ISidesService
        {
        private:
            CSalamanderGeneralAbstract* General;

            int ResolvePanel(SideReference side) const
            {
                if (side == SideReferenceLeft)
                    return PANEL_LEFT;
                if (side == SideReferenceRight)
                    return PANEL_RIGHT;
                if (General == NULL)
                    return 0;

                int source = General->GetSourcePanel();
                if (side == SideReferenceSource)
                    return source;
                if (side == SideReferenceTarget)
                    return source == PANEL_LEFT ? PANEL_RIGHT : PANEL_LEFT;
                return 0;
            }

            static void CopyInfo(
                const CSalamanderPanelTabInfo& source,
                TabInfo* target)
            {
                target->TabId = source.TabId;
                target->PhysicalSide =
                    source.Side == PANEL_RIGHT
                        ? SideReferenceRight
                        : SideReferenceLeft;
                target->Index = source.Index;
                target->PathType = source.PathType;
                target->Flags = source.Flags;
            }

        public:
            explicit SidesService(CSalamanderGeneralAbstract* general)
                : General(general)
            {
            }

            virtual DWORD WINAPI GetVersion() const
            {
                return SALAMATRIX_SIDES_VERSION_1_0;
            }

            virtual SideReference WINAPI ResolveSide(SideReference side) const
            {
                return ResolvePanel(side) == PANEL_RIGHT
                           ? SideReferenceRight
                           : SideReferenceLeft;
            }

            virtual int WINAPI GetTabCount(SideReference side) const
            {
                int panel = ResolvePanel(side);
                return General != NULL && panel != 0
                           ? General->GetPanelTabCount(panel)
                           : 0;
            }

            virtual BOOL WINAPI GetTabInfo(
                SideReference side,
                int index,
                TabInfo* info) const
            {
                if (General == NULL || info == NULL ||
                    info->StructSize < sizeof(*info))
                {
                    return FALSE;
                }

                int panel = ResolvePanel(side);
                CSalamanderPanelTabInfo coreInfo;
                coreInfo.StructSize = sizeof(coreInfo);
                if (panel == 0 ||
                    !General->GetPanelTabInfo(panel, index, &coreInfo))
                {
                    return FALSE;
                }

                CopyInfo(coreInfo, info);
                return TRUE;
            }

            virtual BOOL WINAPI GetActiveTabInfo(
                SideReference side,
                TabInfo* info) const
            {
                int count = GetTabCount(side);
                for (int index = 0; index < count; ++index)
                {
                    TabInfo candidate;
                    if (GetTabInfo(side, index, &candidate) &&
                        (candidate.Flags & TabFlagActiveOnSide) != 0)
                    {
                        if (info == NULL || info->StructSize < sizeof(*info))
                            return FALSE;
                        *info = candidate;
                        return TRUE;
                    }
                }
                return FALSE;
            }

            virtual BOOL WINAPI GetTabInfoById(
                ULONGLONG tabId,
                TabInfo* info) const
            {
                if (tabId == 0 || info == NULL ||
                    info->StructSize < sizeof(*info))
                {
                    return FALSE;
                }

                const SideReference sides[] = {
                    SideReferenceLeft,
                    SideReferenceRight};
                for (int sideIndex = 0;
                     sideIndex < static_cast<int>(_countof(sides));
                     ++sideIndex)
                {
                    int count = GetTabCount(sides[sideIndex]);
                    for (int index = 0; index < count; ++index)
                    {
                        TabInfo candidate;
                        if (GetTabInfo(sides[sideIndex], index, &candidate) &&
                            candidate.TabId == tabId)
                        {
                            *info = candidate;
                            return TRUE;
                        }
                    }
                }
                return FALSE;
            }

            virtual BOOL WINAPI GetTabPath(
                ULONGLONG tabId,
                char* buffer,
                int bufferSize,
                int* pathType) const
            {
                return General != NULL
                           ? General->GetPanelTabPath(
                                 tabId, buffer, bufferSize, pathType)
                           : FALSE;
            }

            virtual BOOL WINAPI ActivateTab(ULONGLONG tabId, BOOL focus)
            {
                return General != NULL
                           ? General->ActivatePanelTab(tabId, focus)
                           : FALSE;
            }

            virtual BOOL WINAPI ChangeActiveTabPath(
                SideReference side,
                const char* path,
                int* failReason)
            {
                int panel = ResolvePanel(side);
                return General != NULL && panel != 0 && path != NULL
                           ? General->ChangePanelPath(panel, path, failReason)
                           : FALSE;
            }

            virtual BOOL WINAPI GetPath(
                SideReference side,
                char* buffer,
                int bufferSize,
                int* pathType) const
            {
                int panel = ResolvePanel(side);
                return General != NULL && panel != 0 && buffer != NULL &&
                               bufferSize > 0
                           ? General->GetPanelPath(
                                 panel, buffer, bufferSize, pathType, NULL)
                           : FALSE;
            }

            virtual int WINAPI GetSelectedItemCount(SideReference side) const
            {
                int panel = ResolvePanel(side);
                int files = 0;
                int directories = 0;
                if (General == NULL || panel == 0 ||
                    !General->GetPanelSelection(
                        panel, &files, &directories))
                    return 0;
                return files + directories;
            }

            static BOOL CopyItemInfo(
                int panel,
                const CFileData* file,
                BOOL isDirectory,
                CSalamanderGeneralAbstract* general,
                ItemInfo* info)
            {
                if (general == NULL || file == NULL || info == NULL ||
                    info->StructSize < sizeof(*info) || file->Name == NULL)
                    return FALSE;
                char panelPath[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
                panelPath[0] = '\0';
                if (!general->GetPanelPath(
                        panel,
                        panelPath,
                        _countof(panelPath),
                        NULL,
                        NULL))
                    return FALSE;
                if (FAILED(StringCchCopyA(
                        info->Name,
                        _countof(info->Name),
                        file->Name)))
                    return FALSE;
                if (FAILED(StringCchCopyA(
                        info->Path,
                        _countof(info->Path),
                        panelPath)))
                    return FALSE;
                size_t length = strlen(info->Path);
                if (length != 0 && info->Path[length - 1] != '\\' &&
                    FAILED(StringCchCatA(
                        info->Path, _countof(info->Path), "\\")))
                    return FALSE;
                if (FAILED(StringCchCatA(
                        info->Path, _countof(info->Path), info->Name)))
                    return FALSE;
                info->Size = file->Size;
                info->Attributes = file->Attr;
                info->IsDirectory = isDirectory;
                return TRUE;
            }

            virtual BOOL WINAPI GetSelectedItem(
                SideReference side,
                int index,
                ItemInfo* info) const
            {
                if (index < 0 || info == NULL)
                    return FALSE;
                int panel = ResolvePanel(side);
                if (General == NULL || panel == 0)
                    return FALSE;
                int cursor = 0;
                BOOL isDirectory = FALSE;
                const CFileData* file = NULL;
                for (int current = 0; current <= index; ++current)
                {
                    file = General->GetPanelSelectedItem(
                        panel, &cursor, &isDirectory);
                    if (file == NULL)
                        return FALSE;
                }
                return CopyItemInfo(
                    panel, file, isDirectory, General, info);
            }

            virtual BOOL WINAPI GetFocusedItem(
                SideReference side,
                ItemInfo* info) const
            {
                if (info == NULL)
                    return FALSE;
                int panel = ResolvePanel(side);
                if (General == NULL || panel == 0)
                    return FALSE;
                BOOL isDirectory = FALSE;
                const CFileData* file = General->GetPanelFocusedItem(
                    panel, &isDirectory);
                return CopyItemInfo(
                    panel, file, isDirectory, General, info);
            }
        };

    } // namespace Sides
} // namespace Salamatrix
