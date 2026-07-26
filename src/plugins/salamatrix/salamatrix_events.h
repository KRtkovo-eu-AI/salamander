// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_events.h
    Runtime-neutral host event subscription service.
*/

#pragma once

#include "salamatrix_sides.h"

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
    // the corresponding operation.  Salamander's legacy PLUGINEVENT contract
    // does not expose equivalent hooks for path/selection/tab changes.
    EventKindSidePathChanged = 8,
    EventKindSideSelectionChanged = 9,
    EventKindSideTabChanged = 10,
    EventKindSideRefreshed = 11
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

    EventPayload()
        : StructSize(sizeof(EventPayload)),
          Kind(EventKindSettingsChanged),
          Parameter(0),
          ActivePanel(0),
          ActiveTabId(0),
          PathType(0)
    {
        Path[0] = 0;
    }
};

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

    EventService(const EventService&);
    EventService& operator=(const EventService&);

public:
    explicit EventService(Sides::ISidesService* sidesService)
        : SubscriberCount(0),
          NextSubscriptionId(0),
          SidesService(sidesService)
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

    virtual BOOL WINAPI Subscribe(
        EventKind kind,
        EventCallback callback,
        void* context,
        ULONGLONG* subscriptionId)
    {
        if (callback == NULL ||
            subscriptionId == NULL ||
            kind < EventKindHostStartup ||
            kind > EventKindSideRefreshed)
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
            payload->StructSize < sizeof(EventPayload))
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
        default:
            return;
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
