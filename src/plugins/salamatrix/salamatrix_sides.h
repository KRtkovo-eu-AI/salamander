// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_sides.h
    Runtime-neutral side and panel-tab snapshot service.
*/

#pragma once

#include "../shared/spl_gen.h"

namespace Salamatrix
{
    namespace Sides
    {

#define SALAMATRIX_SERVICE_SIDES "Salamatrix.Sides"
#define SALAMATRIX_SIDES_VERSION_1_0 0x00010000

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
        };

    } // namespace Sides
} // namespace Salamatrix
