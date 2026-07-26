// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "../precomp.h"
#include "../salamatrix_events.h"

namespace
{
struct CallbackState
{
    int Count;
    Salamatrix::Events::IEventsService* Service;
    ULONGLONG UnsubscribeId;
    Salamatrix::Events::EventKind LastKind;
    DWORD LastParameter;

    CallbackState()
        : Count(0),
          Service(NULL),
          UnsubscribeId(0),
          LastKind(Salamatrix::Events::EventKindSettingsChanged),
          LastParameter(0)
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
    state->LastKind = payload->Kind;
    state->LastParameter = payload->Parameter;
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
            settings.LastKind == Salamatrix::Events::EventKindSettingsChanged &&
            settings.LastParameter == 42,
        "deliver settings event");
    Check(events.GetSubscriptionCount() == 1, "self-unsubscribe during callback");

    Check(events.Publish(&payload) != FALSE, "publish second settings event");
    Check(settings.Count == 1, "unsubscribed callback is not called again");

    payload.Kind = Salamatrix::Events::EventKindPanelsSwapped;
    Check(events.Publish(&payload) != FALSE, "publish panels event");
    Check(panels.Count == 1, "deliver panels event");
    Check(events.GetSubscriptionCount() == 0, "second callback self-unsubscribes");
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
        events.Unsubscribe(ids[index]);
    Check(events.GetSubscriptionCount() == 0, "unsubscribe all callbacks");

    Salamatrix::Events::EventPayload invalid;
    invalid.StructSize = sizeof(invalid) - 1;
    Check(events.Publish(&invalid) == FALSE, "reject undersized payload");
    Check(events.Subscribe(
              static_cast<Salamatrix::Events::EventKind>(99),
              CountCallback,
              &state,
              &rejectedId) == FALSE,
          "reject invalid event kind");

    const Salamatrix::Events::EventKind operationKinds[] = {
        Salamatrix::Events::EventKindSidePathChanged,
        Salamatrix::Events::EventKindSideSelectionChanged,
        Salamatrix::Events::EventKindSideTabChanged,
        Salamatrix::Events::EventKindSideRefreshed};
    for (int index = 0; index < _countof(operationKinds); ++index)
    {
        ULONGLONG operationId = 0;
        Check(
            events.Subscribe(
                operationKinds[index],
                CountCallback,
                &state,
                &operationId) != FALSE,
            "accept shared-side operation event kind");
        events.Unsubscribe(operationId);
    }
}
} // namespace

int main()
{
    TestSubscribePublishAndSelfUnsubscribe();
    TestCapacityAndValidation();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d Salamatrix event test(s) failed.\n", Failures);
        return 1;
    }
    std::puts("All Salamatrix event tests passed.");
    return 0;
}
