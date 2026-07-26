// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_events.h
    Runtime-neutral host event subscription service.
*/

#pragma once

#include "salamatrix_sides.h"
#include <vector>

namespace Salamatrix
{
namespace Events
{

#define SALAMATRIX_SERVICE_EVENTS "Salamatrix.Events"
#define SALAMATRIX_EVENTS_VERSION_1_0 0x00010000

enum EventKind
{
    EventKindHostStartup = 1,
    EventKindHostShutdown = 2,
    EventKindSettingsChanged = 3,
    EventKindConfigurationChanged = 4,
    EventKindColorsChanged = 5,
    EventKindPanelsSwapped = 6,
    EventKindActivePanelChanged = 7,
    // These operation events are emitted when the shared Sides API performs
    // the corresponding operation. They are distinct from core notifications
    // below even when an operation also causes a core notification.
    EventKindSidePathChanged = 8,
    EventKindSideSelectionChanged = 9,
    EventKindSideTabChanged = 10,
    EventKindSideRefreshed = 11,
    // Notifications forwarded from the Salamander core. They are separate
    // from operation events so runtimes can distinguish the source.
    EventKindPathChanged = 12,
    EventKindSelectionChanged = 13,
    EventKindTabChanged = 14,
    // A filesystem change notification delivered by the Salamander core.
    // Parameter is non-zero when the notification covers subdirectories and
    // Path contains the affected UTF-8 path.
    EventKindFileChanged = 15,
    EventKindTabCreated = 16,
    EventKindTabClosed = 17,
    EventKindTabReordered = 18,
    EventKindWindowDetached = 19,
    EventKindWindowAttached = 20
};

struct EventPayload
{
    DWORD StructSize;
    EventKind Kind;
    DWORD Parameter;
    int ActivePanel;
    ULONGLONG ActiveTabId;
    int PathType;
    char Path[32768];
    ULONGLONG ChangedTabId;
    int ChangedTabIndex;
    int PreviousTabIndex;

    EventPayload()
        : StructSize(sizeof(EventPayload)),
          Kind(EventKindSettingsChanged),
          Parameter(0),
          ActivePanel(0),
          ActiveTabId(0),
          PathType(0),
          ChangedTabId(0),
          ChangedTabIndex(-1),
          PreviousTabIndex(-1)
    {
        Path[0] = 0;
    }
};

// The fields through Path are the 1.0 payload prefix. Consumers must accept
// this size so older native providers can still publish legacy events after
// lifecycle fields were appended.
static const DWORD EventPayloadV1Size =
    static_cast<DWORD>(offsetof(EventPayload, ChangedTabId));

typedef BOOL(WINAPI* EventCallback)(
    void* context,
    const EventPayload* payload);

class IEventsService
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual BOOL WINAPI Subscribe(
        EventKind kind,
        EventCallback callback,
        void* context,
        ULONGLONG* subscriptionId) = 0;
    virtual BOOL WINAPI Unsubscribe(ULONGLONG subscriptionId) = 0;
    virtual int WINAPI GetSubscriptionCount() const = 0;
    virtual BOOL WINAPI Publish(const EventPayload* payload) = 0;

protected:
    virtual ~IEventsService() {}
};

// Publish a side operation with a bounded, runtime-neutral snapshot.  This is
// deliberately a free helper instead of a new virtual method so the original
// IEventsService vtable remains ABI-compatible with 1.0 providers.
inline BOOL WINAPI PublishSideOperation(
    IEventsService* events,
    Sides::ISidesService* sides,
    EventKind kind,
    Sides::SideReference side,
    DWORD parameter)
{
    if (events == NULL || sides == NULL ||
        (kind != EventKindSidePathChanged &&
         kind != EventKindSideSelectionChanged &&
         kind != EventKindSideTabChanged &&
         kind != EventKindSideRefreshed))
        return FALSE;

    EventPayload payload;
    payload.Kind = kind;
    payload.Parameter = parameter;
    Sides::SideReference physicalSide = sides->ResolveSide(side);
    payload.ActivePanel = physicalSide == Sides::SideReferenceRight
                              ? PANEL_RIGHT
                              : PANEL_LEFT;

    Sides::TabInfo tab;
    if (sides->GetActiveTabInfo(physicalSide, &tab))
    {
        payload.ActiveTabId = tab.TabId;
        payload.PathType = tab.PathType;
        if (!sides->GetTabPath(
                tab.TabId,
                payload.Path,
                _countof(payload.Path),
                &payload.PathType))
        {
            payload.Path[0] = '\0';
        }
    }
    else
    {
        sides->GetPath(
            physicalSide,
            payload.Path,
            _countof(payload.Path),
            &payload.PathType);
    }
    return events->Publish(&payload);
}

// Forward a core filesystem-change notification without extending the
// IEventsService vtable. This keeps the 1.0 ABI stable for native providers.
inline BOOL WINAPI PublishFileSystemChange(
    IEventsService* events,
    const char* path,
    BOOL includingSubdirectories)
{
    if (events == NULL || path == NULL)
        return FALSE;
    EventPayload payload;
    payload.Kind = EventKindFileChanged;
    payload.Parameter = includingSubdirectories ? 1 : 0;
    size_t length = strlen(path);
    if (length >= _countof(payload.Path))
        length = _countof(payload.Path) - 1;
    memcpy(payload.Path, path, length);
    payload.Path[length] = '\0';
    return events->Publish(&payload);
}

class EventService : public IEventsService
{
private:
    enum
    {
        MaxSubscriptions = 128
    };

    struct Subscriber
    {
        ULONGLONG Id;
        EventKind Kind;
        EventCallback Callback;
        void* Context;

        Subscriber()
            : Id(0),
              Kind(EventKindSettingsChanged),
              Callback(NULL),
              Context(NULL)
        {
        }
    };

    Subscriber Subscribers[MaxSubscriptions];
    int SubscriberCount;
    ULONGLONG NextSubscriptionId;
    mutable CRITICAL_SECTION Lock;
    Sides::ISidesService* SidesService;
    struct TabSnapshot
    {
        ULONGLONG TabId;
        int Index;
        int PathType;
        DWORD Flags;

        TabSnapshot()
            : TabId(0),
              Index(-1),
              PathType(0),
              Flags(Salamatrix::Sides::TabFlagNone)
        {
        }
    };
    std::vector<TabSnapshot> LastTabsLeft;
    std::vector<TabSnapshot> LastTabsRight;
    bool LastTabsLeftInitialized;
    bool LastTabsRightInitialized;

    EventService(const EventService&);
    EventService& operator=(const EventService&);

public:
    explicit EventService(Sides::ISidesService* sidesService)
        : SubscriberCount(0),
          NextSubscriptionId(0),
          SidesService(sidesService),
          LastTabsLeftInitialized(false),
          LastTabsRightInitialized(false)
    {
        InitializeCriticalSection(&Lock);
    }

    virtual ~EventService()
    {
        DeleteCriticalSection(&Lock);
    }

    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_EVENTS_VERSION_1_0;
    }

    const char* GetApiSchema() const
    {
        return "{\"methods\":[\"subscribe\",\"unsubscribe\"],\"eventNames\":[\"hostStartup\",\"hostShutdown\",\"settingsChanged\",\"configurationChanged\",\"colorsChanged\",\"panelsSwapped\",\"activePanelChanged\",\"sidePathChanged\",\"sideSelectionChanged\",\"sideTabChanged\",\"sideRefreshed\",\"pathChanged\",\"selectionChanged\",\"tabChanged\",\"fileChanged\",\"tabCreated\",\"tabClosed\",\"tabReordered\",\"windowDetached\",\"windowAttached\"]}";
    }

    virtual BOOL WINAPI Subscribe(
        EventKind kind,
        EventCallback callback,
        void* context,
        ULONGLONG* subscriptionId)
    {
        if (callback == NULL ||
            subscriptionId == NULL ||
            kind < EventKindHostStartup ||
            kind > EventKindWindowAttached)
        {
            return FALSE;
        }

        EnterCriticalSection(&Lock);
        if (SubscriberCount >= MaxSubscriptions)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }

        ULONGLONG id = ++NextSubscriptionId;
        if (id == 0)
            id = ++NextSubscriptionId;
        Subscriber& subscriber = Subscribers[SubscriberCount++];
        subscriber.Id = id;
        subscriber.Kind = kind;
        subscriber.Callback = callback;
        subscriber.Context = context;
        *subscriptionId = id;
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual BOOL WINAPI Unsubscribe(ULONGLONG subscriptionId)
    {
        if (subscriptionId == 0)
            return FALSE;
        EnterCriticalSection(&Lock);
        for (int index = 0; index < SubscriberCount; ++index)
        {
            if (Subscribers[index].Id == subscriptionId)
            {
                for (int move = index;
                     move + 1 < SubscriberCount;
                     ++move)
                {
                    Subscribers[move] = Subscribers[move + 1];
                }
                Subscribers[--SubscriberCount] = Subscriber();
                LeaveCriticalSection(&Lock);
                return TRUE;
            }
        }
        LeaveCriticalSection(&Lock);
        return FALSE;
    }

    virtual int WINAPI GetSubscriptionCount() const
    {
        EnterCriticalSection(&Lock);
        int count = SubscriberCount;
        LeaveCriticalSection(&Lock);
        return count;
    }

    virtual BOOL WINAPI Publish(const EventPayload* payload)
    {
        if (payload == NULL ||
            payload->StructSize < EventPayloadV1Size ||
            (payload->Kind >= EventKindTabCreated &&
             payload->StructSize < sizeof(EventPayload)))
        {
            return FALSE;
        }

        Subscriber callbacks[MaxSubscriptions];
        int callbackCount = 0;
        EnterCriticalSection(&Lock);
        for (int index = 0; index < SubscriberCount; ++index)
        {
            if (Subscribers[index].Kind == payload->Kind)
                callbacks[callbackCount++] = Subscribers[index];
        }
        LeaveCriticalSection(&Lock);

        for (int index = 0; index < callbackCount; ++index)
        {
            callbacks[index].Callback(
                callbacks[index].Context,
                payload);
        }
        return TRUE;
    }

    BOOL WINAPI CollectTabSnapshot(
        Sides::SideReference side,
        std::vector<TabSnapshot>& snapshot)
    {
        snapshot.clear();
        if (SidesService == NULL)
            return FALSE;

        int count = SidesService->GetTabCount(side);
        if (count <= 0)
            return TRUE;

        snapshot.reserve(static_cast<size_t>(count));
        for (int index = 0; index < count; ++index)
        {
            Sides::TabInfo info;
            if (!SidesService->GetTabInfo(side, index, &info))
                return FALSE;
            TabSnapshot current;
            current.TabId = info.TabId;
            current.Index = info.Index;
            current.PathType = info.PathType;
            current.Flags = info.Flags;
            snapshot.push_back(current);
        }
        return TRUE;
    }

    int WINAPI FindTabIndex(
        const std::vector<TabSnapshot>& snapshot,
        ULONGLONG tabId) const
    {
        for (size_t index = 0; index < snapshot.size(); ++index)
        {
            if (snapshot[index].TabId == tabId)
                return static_cast<int>(index);
        }
        return -1;
    }

    void WINAPI FillActiveSideMetadata(
        EventPayload& payload,
        Sides::SideReference side)
    {
        payload.ActiveTabId = 0;
        payload.PathType = 0;
        payload.Path[0] = 0;
        if (SidesService == NULL)
            return;

        Sides::TabInfo tab;
        if (SidesService->GetActiveTabInfo(side, &tab))
        {
            payload.ActiveTabId = tab.TabId;
            payload.PathType = tab.PathType;
            SidesService->GetTabPath(
                tab.TabId,
                payload.Path,
                _countof(payload.Path),
                &payload.PathType);
        }
        else
        {
            SidesService->GetPath(
                side,
                payload.Path,
                _countof(payload.Path),
                &payload.PathType);
        }
    }

    void WINAPI PublishTabLifecycleEvents(
        Sides::SideReference side,
        const std::vector<TabSnapshot>& current)
    {
        std::vector<TabSnapshot>& previous =
            (side == Sides::SideReferenceRight) ? LastTabsRight : LastTabsLeft;

        EventPayload payload;
        payload.ActivePanel =
            side == Sides::SideReferenceRight ? PANEL_RIGHT : PANEL_LEFT;
        FillActiveSideMetadata(payload, side);

        for (size_t index = 0; index < previous.size(); ++index)
        {
            const TabSnapshot& oldTab = previous[index];
            int newIndex = FindTabIndex(current, oldTab.TabId);
            if (newIndex < 0)
            {
                payload.Kind = EventKindTabClosed;
                payload.ChangedTabId = oldTab.TabId;
                payload.ChangedTabIndex = oldTab.Index;
                payload.PreviousTabIndex = oldTab.Index;
                Publish(&payload);
            }
            else
            {
                int oldFlags = oldTab.Flags & Salamatrix::Sides::TabFlagDetached;
                int newFlags = current[newIndex].Flags &
                               Salamatrix::Sides::TabFlagDetached;
                if (oldFlags != newFlags)
                {
                    if (oldFlags == 0)
                        payload.Kind = EventKindWindowDetached;
                    else
                        payload.Kind = EventKindWindowAttached;
                    payload.ChangedTabId = oldTab.TabId;
                    payload.ChangedTabIndex = newIndex;
                    payload.PreviousTabIndex = oldTab.Index;
                    Publish(&payload);
                }
            }
        }

        for (size_t index = 0; index < current.size(); ++index)
        {
            const TabSnapshot& newTab = current[index];
            int oldIndex = FindTabIndex(previous, newTab.TabId);
            if (oldIndex < 0)
            {
                payload.Kind = EventKindTabCreated;
                payload.ChangedTabId = newTab.TabId;
                payload.ChangedTabIndex = newTab.Index;
                payload.PreviousTabIndex = -1;
                Publish(&payload);
            }
        }

        if (current.size() == previous.size())
        {
            bool reordered = FALSE;
            for (size_t index = 0; index < current.size(); ++index)
            {
                int oldIndex = FindTabIndex(previous, current[index].TabId);
                if (oldIndex >= 0 &&
                    oldIndex != static_cast<int>(index))
                {
                    payload.Kind = EventKindTabReordered;
                    payload.ChangedTabId = current[index].TabId;
                    payload.ChangedTabIndex = current[index].Index;
                    payload.PreviousTabIndex = oldIndex;
                    Publish(&payload);
                    reordered = TRUE;
                    break;
                }
            }
            if (!reordered && current.size() > 1)
            {
                for (size_t index = 0; index + 1 < current.size(); ++index)
                {
                    int oldIndex = FindTabIndex(previous, current[index].TabId);
                    if (oldIndex != static_cast<int>(index))
                    {
                        payload.Kind = EventKindTabReordered;
                        payload.ChangedTabId = current[index].TabId;
                        payload.ChangedTabIndex = current[index].Index;
                        payload.PreviousTabIndex = oldIndex;
                        Publish(&payload);
                        break;
                    }
                }
            }
        }
    }

    void WINAPI HandleTabChangedLifecycle(Sides::SideReference side)
    {
        std::vector<TabSnapshot> current;
        if (!CollectTabSnapshot(side, current))
            return;

        bool& initialized =
            side == Sides::SideReferenceRight ? LastTabsRightInitialized
                                               : LastTabsLeftInitialized;
        if (!initialized)
        {
            if (side == Sides::SideReferenceRight)
                LastTabsRight = current;
            else
                LastTabsLeft = current;
            initialized = true;
            return;
        }

        PublishTabLifecycleEvents(side, current);

        if (side == Sides::SideReferenceRight)
            LastTabsRight = current;
        else
            LastTabsLeft = current;
    }

    void WINAPI PublishHostEvent(int event, DWORD parameter)
    {
        EventPayload payload;
        payload.Parameter = parameter;
        switch (event)
        {
        case PLUGINEVENT_COLORSCHANGED:
            payload.Kind = EventKindColorsChanged;
            break;
        case PLUGINEVENT_CONFIGURATIONCHANGED:
            payload.Kind = EventKindConfigurationChanged;
            break;
        case PLUGINEVENT_PANELSSWAPPED:
            payload.Kind = EventKindPanelsSwapped;
            break;
        case PLUGINEVENT_PANELACTIVATED:
            payload.Kind = EventKindActivePanelChanged;
            payload.ActivePanel = parameter == PANEL_RIGHT
                                      ? PANEL_RIGHT
                                      : PANEL_LEFT;
            if (SidesService != NULL)
            {
                Sides::SideReference side =
                    payload.ActivePanel == PANEL_RIGHT
                        ? Sides::SideReferenceRight
                        : Sides::SideReferenceLeft;
                Sides::TabInfo tab;
                if (SidesService->GetActiveTabInfo(side, &tab))
                {
                    payload.ActiveTabId = tab.TabId;
                    payload.PathType = tab.PathType;
                    SidesService->GetTabPath(
                        tab.TabId,
                        payload.Path,
                        _countof(payload.Path),
                        NULL);
                }
            }
            break;
        case PLUGINEVENT_SETTINGCHANGE:
            payload.Kind = EventKindSettingsChanged;
            break;
        case PLUGINEVENT_PATHCHANGED:
            payload.Kind = EventKindPathChanged;
            payload.ActivePanel = parameter == PANEL_RIGHT
                                      ? PANEL_RIGHT
                                      : PANEL_LEFT;
            break;
        case PLUGINEVENT_SELECTIONCHANGED:
            payload.Kind = EventKindSelectionChanged;
            payload.ActivePanel = parameter == PANEL_RIGHT
                                      ? PANEL_RIGHT
                                      : PANEL_LEFT;
            break;
        case PLUGINEVENT_TABCHANGED:
            payload.Kind = EventKindTabChanged;
            payload.ActivePanel = parameter == PANEL_RIGHT
                                      ? PANEL_RIGHT
                                      : PANEL_LEFT;
            if (SidesService != NULL)
            {
                HandleTabChangedLifecycle(
                    payload.ActivePanel == PANEL_RIGHT
                        ? Sides::SideReferenceRight
                        : Sides::SideReferenceLeft);
            }
            break;
        default:
            return;
        }
        if ((payload.Kind == EventKindPathChanged ||
             payload.Kind == EventKindSelectionChanged ||
             payload.Kind == EventKindTabChanged) &&
            SidesService != NULL)
        {
            Sides::SideReference side =
                payload.ActivePanel == PANEL_RIGHT
                    ? Sides::SideReferenceRight
                    : Sides::SideReferenceLeft;
            FillActiveSideMetadata(payload, side);
        }
        Publish(&payload);
    }

    void WINAPI PublishLifecycle(EventKind kind)
    {
        if (kind != EventKindHostStartup &&
            kind != EventKindHostShutdown)
        {
            return;
        }
        EventPayload payload;
        payload.Kind = kind;
        Publish(&payload);
    }
};

} // namespace Events
} // namespace Salamatrix
