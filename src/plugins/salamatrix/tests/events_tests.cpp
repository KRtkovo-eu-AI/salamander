// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <string>
#include <vector>

#include "../precomp.h"
#include "../salamatrix_events.h"

namespace
{
struct TabState
{
    ULONGLONG TabId;
    int Index;
    int PathType;
    DWORD Flags;
    const char* Path;
};

class MockSidesService : public Salamatrix::Sides::ISidesService
{
private:
    std::vector<TabState> LeftTabs;
    std::vector<TabState> RightTabs;

    std::vector<TabState>& Tabs(Salamatrix::Sides::SideReference side)
    {
        return side == Salamatrix::Sides::SideReferenceRight ? RightTabs
                                                            : LeftTabs;
    }

    const std::vector<TabState>& Tabs(
        Salamatrix::Sides::SideReference side) const
    {
        return side == Salamatrix::Sides::SideReferenceRight ? RightTabs
                                                            : LeftTabs;
    }

    int ActiveIndex(const std::vector<TabState>& tabs) const
    {
        for (size_t index = 0; index < tabs.size(); ++index)
        {
            if ((tabs[index].Flags & Salamatrix::Sides::TabFlagActiveOnSide) !=
                0)
                return static_cast<int>(index);
        }
        return -1;
    }

    void CopyTabs(
        Salamatrix::Sides::SideReference side,
        const std::vector<TabState>& tabs)
    {
        std::vector<TabState>& target = Tabs(side);
        target.clear();
        for (size_t index = 0; index < tabs.size(); ++index)
        {
            TabState entry = tabs[index];
            entry.Index = static_cast<int>(index);
            target.push_back(entry);
        }
        if (ActiveIndex(target) < 0 && !target.empty())
        {
            target[0].Flags |= Salamatrix::Sides::TabFlagActiveOnSide;
        }
    }

public:
    virtual DWORD WINAPI GetVersion() const
    {
        return 0x00010000;
    }

    void SetLeftTabs(const std::vector<TabState>& tabs)
    {
        CopyTabs(Salamatrix::Sides::SideReferenceLeft, tabs);
    }

    virtual Salamatrix::Sides::SideReference WINAPI ResolveSide(
        Salamatrix::Sides::SideReference side) const
    {
        if (side == Salamatrix::Sides::SideReferenceRight)
            return Salamatrix::Sides::SideReferenceRight;
        return Salamatrix::Sides::SideReferenceLeft;
    }

    virtual int WINAPI GetTabCount(
        Salamatrix::Sides::SideReference side) const
    {
        return static_cast<int>(Tabs(side).size());
    }

    virtual BOOL WINAPI GetTabInfo(
        Salamatrix::Sides::SideReference side,
        int index,
        Salamatrix::Sides::TabInfo* info) const
    {
        const std::vector<TabState>& sideTabs = Tabs(side);
        if (info == NULL || info->StructSize < sizeof(*info) ||
            index < 0 || static_cast<size_t>(index) >= sideTabs.size())
            return FALSE;

        info->TabId = sideTabs[index].TabId;
        info->PhysicalSide = side;
        info->Index = sideTabs[index].Index;
        info->PathType = sideTabs[index].PathType;
        info->Flags = sideTabs[index].Flags;
        return TRUE;
    }

    virtual BOOL WINAPI GetTabInfoById(
        ULONGLONG tabId,
        Salamatrix::Sides::TabInfo* info) const
    {
        for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
        {
            Salamatrix::Sides::SideReference side =
                sideIndex == 0 ? Salamatrix::Sides::SideReferenceLeft
                               : Salamatrix::Sides::SideReferenceRight;
            const std::vector<TabState>& sideTabs = Tabs(side);
            for (size_t index = 0; index < sideTabs.size(); ++index)
            {
                if (sideTabs[index].TabId == tabId)
                {
                    if (info == NULL || info->StructSize < sizeof(*info))
                        return FALSE;
                    info->TabId = sideTabs[index].TabId;
                    info->PhysicalSide = side;
                    info->Index = sideTabs[index].Index;
                    info->PathType = sideTabs[index].PathType;
                    info->Flags = sideTabs[index].Flags;
                    return TRUE;
                }
            }
        }
        return FALSE;
    }

    virtual BOOL WINAPI GetActiveTabInfo(
        Salamatrix::Sides::SideReference side,
        Salamatrix::Sides::TabInfo* info) const
    {
        const std::vector<TabState>& sideTabs = Tabs(side);
        int active = ActiveIndex(sideTabs);
        if (active < 0 || info == NULL || info->StructSize < sizeof(*info))
            return FALSE;
        return GetTabInfo(side, active, info);
    }

    virtual BOOL WINAPI GetTabPath(
        ULONGLONG tabId,
        char* buffer,
        int bufferSize,
        int* pathType) const
    {
        const TabState* match = NULL;
        for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
        {
            Salamatrix::Sides::SideReference side =
                sideIndex == 0 ? Salamatrix::Sides::SideReferenceLeft
                               : Salamatrix::Sides::SideReferenceRight;
            const std::vector<TabState>& sideTabs = Tabs(side);
            for (size_t index = 0; index < sideTabs.size(); ++index)
            {
                if (sideTabs[index].TabId == tabId)
                {
                    match = &sideTabs[index];
                    break;
                }
            }
            if (match != NULL)
                break;
        }
        if (match == NULL || match->Path == NULL || buffer == NULL ||
            bufferSize <= 0)
            return FALSE;
        size_t length = strlen(match->Path);
        if (static_cast<int>(length) >= bufferSize)
            length = static_cast<size_t>(bufferSize - 1);
        memcpy(buffer, match->Path, length);
        buffer[length] = 0;
        if (pathType != NULL)
            *pathType = match->PathType;
        return TRUE;
    }

    virtual BOOL WINAPI ActivateTab(ULONGLONG tabId, BOOL focus)
    {
        for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
        {
            Salamatrix::Sides::SideReference side =
                sideIndex == 0 ? Salamatrix::Sides::SideReferenceLeft
                               : Salamatrix::Sides::SideReferenceRight;
            std::vector<TabState>& sideTabs = Tabs(side);
            for (size_t index = 0; index < sideTabs.size(); ++index)
            {
                if (sideTabs[index].TabId == tabId)
                {
                    for (size_t other = 0; other < sideTabs.size(); ++other)
                    {
                        sideTabs[other].Flags &=
                            ~Salamatrix::Sides::TabFlagActiveOnSide;
                    }
                    sideTabs[index].Flags |=
                        Salamatrix::Sides::TabFlagActiveOnSide;
                    return TRUE;
                }
            }
        }
        (void)focus;
        return FALSE;
    }

    virtual BOOL WINAPI ChangeActiveTabPath(
        Salamatrix::Sides::SideReference,
        const char*,
        int*) { return FALSE; }

    virtual BOOL WINAPI GetPath(
        Salamatrix::Sides::SideReference side,
        char* buffer,
        int bufferSize,
        int* pathType) const
    {
        const std::vector<TabState>& sideTabs = Tabs(side);
        if (buffer == NULL || bufferSize <= 0)
            return FALSE;
        if (pathType != NULL)
            *pathType = sideTabs.empty() ? 0 : sideTabs[0].PathType;
        if (sideTabs.empty())
        {
            buffer[0] = 0;
            return TRUE;
        }

        const char* path = sideTabs[0].Path;
        if (path == NULL)
            return FALSE;
        size_t length = strlen(path);
        if (static_cast<int>(length) >= bufferSize)
            length = static_cast<size_t>(bufferSize - 1);
        memcpy(buffer, path, length);
        buffer[length] = 0;
        return TRUE;
    }

    virtual int WINAPI GetSelectedItemCount(Salamatrix::Sides::SideReference) const
    {
        return 0;
    }

    virtual BOOL WINAPI GetSelectedItem(
        Salamatrix::Sides::SideReference,
        int,
        Salamatrix::Sides::ItemInfo*) const
    {
        return FALSE;
    }

    virtual BOOL WINAPI GetFocusedItem(
        Salamatrix::Sides::SideReference,
        Salamatrix::Sides::ItemInfo*) const
    {
        return FALSE;
    }

    virtual BOOL WINAPI Refresh(
        Salamatrix::Sides::SideReference,
        BOOL,
        BOOL) { return FALSE; }

    virtual BOOL WINAPI SetItemSelected(
        Salamatrix::Sides::SideReference,
        int,
        BOOL,
        BOOL) { return FALSE; }

    virtual BOOL WINAPI SelectAll(
        Salamatrix::Sides::SideReference,
        BOOL,
        BOOL) { return FALSE; }

    virtual BOOL WINAPI FocusItem(
        Salamatrix::Sides::SideReference,
        int,
        BOOL) { return FALSE; }

    virtual BOOL WINAPI CreateTab(
        Salamatrix::Sides::SideReference,
        const char*,
        int,
        ULONGLONG*) { return FALSE; }

    virtual BOOL WINAPI CloseTab(ULONGLONG) { return FALSE; }

    virtual BOOL WINAPI ReorderTab(ULONGLONG, int) { return FALSE; }

    virtual BOOL WINAPI MoveTab(
        ULONGLONG,
        Salamatrix::Sides::SideReference,
        int) { return FALSE; }

    virtual BOOL WINAPI SetPanelsDetached(BOOL) { return FALSE; }
};

struct CallbackState
{
    int Count;
    Salamatrix::Events::EventPayload LastPayload;
    Salamatrix::Events::IEventsService* Service;
    ULONGLONG UnsubscribeId;
    std::vector<Salamatrix::Events::EventPayload> Events;

    CallbackState()
        : Count(0),
          LastPayload(),
          Service(NULL),
          UnsubscribeId(0)
    {
    }
};

BOOL WINAPI CountCallback(
    void* context,
    const Salamatrix::Events::EventPayload* payload)
{
    CallbackState* state = static_cast<CallbackState*>(context);
    if (state == NULL || payload == NULL)
        return FALSE;
    ++state->Count;
    state->LastPayload = Salamatrix::Events::EventPayload();
    size_t copySize = payload->StructSize;
    if (copySize > sizeof(Salamatrix::Events::EventPayload))
        copySize = sizeof(Salamatrix::Events::EventPayload);
    memcpy(&state->LastPayload, payload, copySize);
    state->Events.push_back(*payload);
    if (state->Service != NULL && state->UnsubscribeId != 0)
    {
        state->Service->Unsubscribe(state->UnsubscribeId);
        state->UnsubscribeId = 0;
    }
    return TRUE;
}

int Failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        ++Failures;
    }
}

void TestSubscribePublishAndSelfUnsubscribe()
{
    Salamatrix::Events::EventService events(NULL);
    CallbackState settings;
    CallbackState panels;
    settings.Service = &events;
    panels.Service = &events;

    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindSettingsChanged,
            CountCallback,
            &settings,
            &settings.UnsubscribeId) != FALSE,
        "subscribe settings callback");
    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindPanelsSwapped,
            CountCallback,
            &panels,
            &panels.UnsubscribeId) != FALSE,
        "subscribe panels callback");
    Check(events.GetSubscriptionCount() == 2, "count subscriptions");

    Salamatrix::Events::EventPayload payload;
    payload.Kind = Salamatrix::Events::EventKindSettingsChanged;
    payload.Parameter = 42;
    Check(events.Publish(&payload) != FALSE, "publish settings event");
    Check(
        settings.Count == 1 &&
            settings.LastPayload.Kind == Salamatrix::Events::EventKindSettingsChanged &&
            settings.LastPayload.Parameter == 42,
        "deliver settings event");
    Check(events.GetSubscriptionCount() == 1, "self-unsubscribe during callback");

    Check(events.Publish(&payload) != FALSE, "publish second settings event");
    Check(settings.Count == 1, "unsubscribed callback is not called again");

    payload.Kind = Salamatrix::Events::EventKindPanelsSwapped;
    Check(events.Publish(&payload) != FALSE, "publish panels event");
    Check(panels.Count == 1, "deliver panels event");
    Check(events.GetSubscriptionCount() == 0, "second callback self-unsubscribes");
}

void TestLegacyPayloadPrefixRemainsPublishable()
{
    Salamatrix::Events::EventService events(NULL);
    CallbackState state;
    ULONGLONG subscriptionId = 0;
    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindSettingsChanged,
            CountCallback,
            &state,
            &subscriptionId) != FALSE,
        "subscribe legacy payload test");

    Salamatrix::Events::EventPayload payload;
    payload.StructSize = Salamatrix::Events::EventPayloadV1Size;
    payload.Kind = Salamatrix::Events::EventKindSettingsChanged;
    Check(events.Publish(&payload) != FALSE && state.Count == 1,
          "publish legacy event payload prefix");
}

void TestSchemaContainsLifecycleEvents()
{
    Salamatrix::Events::EventService events(NULL);
    const char* schema = events.GetApiSchema();
    Check(
        std::string(schema).find("\"tabCreated\"") != std::string::npos,
        "schema includes tabCreated");
    Check(
        std::string(schema).find("\"tabClosed\"") != std::string::npos,
        "schema includes tabClosed");
    Check(
        std::string(schema).find("\"tabReordered\"") != std::string::npos,
        "schema includes tabReordered");
    Check(
        std::string(schema).find("\"windowDetached\"") != std::string::npos,
        "schema includes windowDetached");
    Check(
        std::string(schema).find("\"windowAttached\"") != std::string::npos,
        "schema includes windowAttached");
}

void TestCapacityAndValidation()
{
    Salamatrix::Events::EventService events(NULL);
    CallbackState state;
    ULONGLONG ids[128];
    for (int index = 0; index < _countof(ids); ++index)
    {
        ids[index] = 0;
        Check(
            events.Subscribe(
                Salamatrix::Events::EventKindColorsChanged,
                CountCallback,
                &state,
                &ids[index]) != FALSE,
            "subscribe within capacity");
    }
    ULONGLONG rejectedId = 0;
    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindColorsChanged,
            CountCallback,
            &state,
            &rejectedId) == FALSE,
        "reject subscription over capacity");
    Check(events.GetSubscriptionCount() == 128, "capacity count");
    for (int index = 0; index < _countof(ids); ++index)
    {
        events.Unsubscribe(ids[index]);
    }
    Check(events.GetSubscriptionCount() == 0, "unsubscribe all callbacks");

    Salamatrix::Events::EventPayload invalid;
    invalid.StructSize = Salamatrix::Events::EventPayloadV1Size - 1;
    Check(events.Publish(&invalid) == FALSE, "reject undersized payload");
    Check(
        events.Subscribe(
            static_cast<Salamatrix::Events::EventKind>(99),
            CountCallback,
            &state,
            &rejectedId) == FALSE,
        "reject invalid event kind");

    const Salamatrix::Events::EventKind operationKinds[] = {
        Salamatrix::Events::EventKindSidePathChanged,
        Salamatrix::Events::EventKindSideSelectionChanged,
        Salamatrix::Events::EventKindSideTabChanged,
        Salamatrix::Events::EventKindSideRefreshed,
        Salamatrix::Events::EventKindPathChanged,
        Salamatrix::Events::EventKindSelectionChanged,
        Salamatrix::Events::EventKindTabChanged,
        Salamatrix::Events::EventKindTabCreated,
        Salamatrix::Events::EventKindTabClosed,
        Salamatrix::Events::EventKindTabReordered,
        Salamatrix::Events::EventKindWindowDetached,
        Salamatrix::Events::EventKindWindowAttached};
    for (int index = 0; index < _countof(operationKinds); ++index)
    {
        ULONGLONG operationId = 0;
        Check(
            events.Subscribe(
                operationKinds[index],
                CountCallback,
                &state,
                &operationId) != FALSE,
            "accept expanded and legacy event kind");
        events.Unsubscribe(operationId);
    }

    ULONGLONG fileId = 0;
    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindFileChanged,
            CountCallback,
            &state,
            &fileId) != FALSE,
        "accept filesystem change event kind");
    events.Unsubscribe(fileId);
}

void TestFilesystemChangeHelper()
{
    Salamatrix::Events::EventService events(NULL);
    CallbackState state;
    state.Service = &events;
    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindFileChanged,
            CountCallback,
            &state,
            &state.UnsubscribeId) != FALSE,
        "subscribe filesystem change event");
    Check(
        Salamatrix::Events::PublishFileSystemChange(
            &events, "C:/build/output", TRUE) != FALSE,
        "publish filesystem change event");
    Check(
        state.Count == 1 &&
            state.LastPayload.Kind == Salamatrix::Events::EventKindFileChanged &&
            state.LastPayload.Parameter == 1 &&
            std::string(state.LastPayload.Path) == "C:/build/output",
        "deliver filesystem change path and scope");
}

void TestCoreNotifications()
{
    Salamatrix::Events::EventService events(NULL);
    const int hostEvents[] = {
        PLUGINEVENT_PATHCHANGED,
        PLUGINEVENT_SELECTIONCHANGED,
        PLUGINEVENT_TABCHANGED};
    const Salamatrix::Events::EventKind kinds[] = {
        Salamatrix::Events::EventKindPathChanged,
        Salamatrix::Events::EventKindSelectionChanged,
        Salamatrix::Events::EventKindTabChanged};
    for (int index = 0; index < _countof(hostEvents); ++index)
    {
        CallbackState state;
        state.Service = &events;
        Check(
            events.Subscribe(
                kinds[index],
                CountCallback,
                &state,
                &state.UnsubscribeId) != FALSE,
            "subscribe core notification");
        events.PublishHostEvent(hostEvents[index], PANEL_RIGHT);
        Check(
            state.Count == 1 &&
                state.LastPayload.Kind == kinds[index] &&
                state.LastPayload.Parameter == PANEL_RIGHT,
            "deliver core notification");
    }
}

void TestTabLifecycleEvents()
{
    MockSidesService sides;
    Salamatrix::Events::EventService events(&sides);
    CallbackState state;
    state.Service = &events;

    const Salamatrix::Events::EventKind eventKinds[] = {
        Salamatrix::Events::EventKindTabCreated,
        Salamatrix::Events::EventKindTabClosed,
        Salamatrix::Events::EventKindTabReordered,
        Salamatrix::Events::EventKindWindowDetached,
        Salamatrix::Events::EventKindWindowAttached,
        Salamatrix::Events::EventKindTabChanged};
    ULONGLONG ids[] = {0, 0, 0, 0, 0, 0};
    for (int index = 0; index < _countof(eventKinds); ++index)
    {
        Check(
            events.Subscribe(
                eventKinds[index],
                CountCallback,
                &state,
                &ids[index]) != FALSE,
            "subscribe tab lifecycle event kind");
    }

    const TabState baseline[] = {
        {100, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide, "/left/tab/100"},
        {101, -1, 1, Salamatrix::Sides::TabFlagNone, "/left/tab/101"}};
    sides.SetLeftTabs(
        std::vector<TabState>(baseline, baseline + _countof(baseline)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Count == 1 &&
            state.Events.size() == 1 &&
            state.Events[0].Kind == Salamatrix::Events::EventKindTabChanged,
        "baseline update emits only legacy tabChanged");

    state.Count = 0;
    state.Events.clear();
    const TabState created[] = {
        {100, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide, "/left/tab/100"},
        {102, -1, 1, Salamatrix::Sides::TabFlagNone, "/left/tab/102"},
        {101, -1, 1, Salamatrix::Sides::TabFlagNone, "/left/tab/101"}};
    sides.SetLeftTabs(
        std::vector<TabState>(created, created + _countof(created)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Count == 2 && state.Events.size() >= 2 &&
            state.Events[0].Kind == Salamatrix::Events::EventKindTabCreated &&
            state.Events[0].ChangedTabId == 102 &&
            state.Events[0].ChangedTabIndex == 1 &&
            state.Events[0].PreviousTabIndex == -1,
        "tabCreated lifecycle event");
    Check(
        state.Events[1].Kind == Salamatrix::Events::EventKindTabChanged &&
            state.Events[1].ActiveTabId == 100,
        "legacy tabChanged follows lifecycle");

    state.Count = 0;
    state.Events.clear();
    const TabState closed[] = {
        {100, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide, "/left/tab/100"},
        {101, -1, 1, Salamatrix::Sides::TabFlagNone, "/left/tab/101"}};
    sides.SetLeftTabs(
        std::vector<TabState>(closed, closed + _countof(closed)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Count == 2 && state.Events.size() >= 2 &&
            state.Events[0].Kind == Salamatrix::Events::EventKindTabClosed &&
            state.Events[0].ChangedTabId == 102 &&
            state.Events[0].ChangedTabIndex == 1 &&
            state.Events[0].PreviousTabIndex == 1,
        "tabClosed lifecycle event");

    state.Count = 0;
    state.Events.clear();
    const TabState reorderAfter[] = {
        {101, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide, "/left/tab/101"},
        {100, -1, 1, Salamatrix::Sides::TabFlagNone, "/left/tab/100"}};
    sides.SetLeftTabs(
        std::vector<TabState>(
            reorderAfter,
            reorderAfter + _countof(reorderAfter)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Count == 2 && state.Events.size() >= 2 &&
            state.Events[0].Kind == Salamatrix::Events::EventKindTabReordered &&
            state.Events[0].ChangedTabId == 101 &&
            state.Events[0].ChangedTabIndex == 0 &&
            state.Events[0].PreviousTabIndex == 1,
        "tabReordered lifecycle event");

    state.Count = 0;
    state.Events.clear();
    const TabState detached[] = {
        {101, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide, "/left/tab/101"},
        {100, -1, 1, Salamatrix::Sides::TabFlagDetached, "/left/tab/100"}};
    sides.SetLeftTabs(
        std::vector<TabState>(detached, detached + _countof(detached)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Count == 2 && state.Events.size() >= 2 &&
            state.Events[0].Kind ==
                Salamatrix::Events::EventKindWindowDetached &&
            state.Events[0].ChangedTabId == 100 &&
            state.Events[0].ChangedTabIndex == 1 &&
            state.Events[0].PreviousTabIndex == 1,
        "windowDetached lifecycle event");

    state.Count = 0;
    state.Events.clear();
    const TabState attached[] = {
        {101, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide, "/left/tab/101"},
        {100, -1, 1, Salamatrix::Sides::TabFlagNone, "/left/tab/100"}};
    sides.SetLeftTabs(
        std::vector<TabState>(attached, attached + _countof(attached)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Count == 2 && state.Events.size() >= 2 &&
            state.Events[0].Kind ==
                Salamatrix::Events::EventKindWindowAttached &&
            state.Events[0].ChangedTabId == 100 &&
            state.Events[0].ChangedTabIndex == 1 &&
            state.Events[0].PreviousTabIndex == 1,
        "windowAttached lifecycle event");

    state.Count = 0;
    state.Events.clear();
    const TabState closedForLife[] = {
        {101, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide, "/left/tab/101"}};
    sides.SetLeftTabs(
        std::vector<TabState>(
            closedForLife,
            closedForLife + _countof(closedForLife)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Count == 2 && state.Events.size() >= 2 &&
            state.Events[0].Kind == Salamatrix::Events::EventKindTabClosed &&
            state.Events[0].ChangedTabId == 100 &&
            state.Events[0].ChangedTabIndex == 1 &&
            state.Events[0].PreviousTabIndex == 1,
        "tabClosed lifecycle event");
}

void TestZeroToTabsLifecycle()
{
    MockSidesService sides;
    Salamatrix::Events::EventService events(&sides);
    CallbackState state;
    state.Service = &events;
    ULONGLONG createdId = 0;
    ULONGLONG changedId = 0;
    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindTabCreated,
            CountCallback,
            &state,
            &createdId) != FALSE,
        "subscribe zero-to-tabs created event");
    Check(
        events.Subscribe(
            Salamatrix::Events::EventKindTabChanged,
            CountCallback,
            &state,
            &changedId) != FALSE,
        "subscribe zero-to-tabs legacy event");

    sides.SetLeftTabs(std::vector<TabState>());
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    state.Count = 0;
    state.Events.clear();

    const TabState firstTab[] = {
        {200, -1, 1, Salamatrix::Sides::TabFlagActiveOnSide,
         "/left/tab/200"}};
    sides.SetLeftTabs(
        std::vector<TabState>(firstTab, firstTab + _countof(firstTab)));
    events.PublishHostEvent(PLUGINEVENT_TABCHANGED, PANEL_LEFT);
    Check(
        state.Events.size() == 2 &&
            state.Events[0].Kind == Salamatrix::Events::EventKindTabCreated &&
            state.Events[0].ChangedTabId == 200 &&
            state.Events[0].ChangedTabIndex == 0 &&
            state.Events[0].PreviousTabIndex == -1,
        "tabCreated lifecycle event after empty baseline");
    Check(
        state.Events[1].Kind == Salamatrix::Events::EventKindTabChanged,
        "legacy tabChanged follows empty-baseline lifecycle");
}

} // namespace

int main()
{
    TestSubscribePublishAndSelfUnsubscribe();
    TestLegacyPayloadPrefixRemainsPublishable();
    TestCapacityAndValidation();
    TestSchemaContainsLifecycleEvents();
    TestCoreNotifications();
    TestFilesystemChangeHelper();
    TestTabLifecycleEvents();
    TestZeroToTabsLifecycle();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d Salamatrix event test(s) failed.\n", Failures);
        return 1;
    }
    std::puts("All Salamatrix event tests passed.");
    return 0;
}
