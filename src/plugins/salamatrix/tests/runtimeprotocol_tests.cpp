// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "../precomp.h"
#include "../salamatrix_ai.h"
#include "../salamatrix_commands.h"
#include "../salamatrix_runtime_api.h"
#include "../salamatrix_runtime_protocol.h"

namespace
{
int Failures = 0;

class TestAssistantProvider : public Salamatrix::AI::IAssistantProvider
{
private:
    Salamatrix::AI::AssistantProviderDescriptor Descriptor;

public:
    TestAssistantProvider()
    {
        Descriptor.ProviderId = "test";
        Descriptor.DisplayName = "Test";
        Descriptor.ProviderVersion = 0x00010000;
        Descriptor.Flags = 0;
    }

    virtual const Salamatrix::AI::AssistantProviderDescriptor* WINAPI GetDescriptor() const
    {
        return &Descriptor;
    }

    virtual BOOL WINAPI IsAvailable() const
    {
        return TRUE;
    }

    virtual BOOL WINAPI Generate(
        const Salamatrix::AI::AssistantRequest*,
        Salamatrix::AI::AssistantResponse* response)
    {
        const char result[] =
            "{\"title\":\"Test\",\"description\":\"Test script\","
            "\"capabilities\":[],\"estimatedEffects\":{},"
            "\"script\":\"pass\",\"runtime\":\"Python.CPython\"}";
        memcpy(response->ResponseJson, result, sizeof(result));
        response->OutputLength = sizeof(result) - 1;
        response->Status = Salamatrix::AI::AssistantStatusSucceeded;
        return TRUE;
    }
};

class FailingAssistantProvider : public Salamatrix::AI::IAssistantProvider
{
private:
    Salamatrix::AI::AssistantProviderDescriptor Descriptor;

public:
    FailingAssistantProvider()
    {
        Descriptor.ProviderId = "failing";
        Descriptor.DisplayName = "Failing test provider";
        Descriptor.ProviderVersion = 0x00010000;
        Descriptor.Flags = 0;
    }

    virtual const Salamatrix::AI::AssistantProviderDescriptor* WINAPI
    GetDescriptor() const { return &Descriptor; }
    virtual BOOL WINAPI IsAvailable() const { return TRUE; }
    virtual BOOL WINAPI Generate(
        const Salamatrix::AI::AssistantRequest*,
        Salamatrix::AI::AssistantResponse* response)
    {
        response->Status = Salamatrix::AI::AssistantStatusFailed;
        response->ErrorCode = E_FAIL;
        return FALSE;
    }
};

class TestRuntimeAdapter : public Salamatrix::Runtime::IRuntimeAdapter
{
private:
    Salamatrix::Runtime::RuntimeAdapterDescriptor Descriptor;

public:
    TestRuntimeAdapter()
    {
        Descriptor.RuntimeId = "Test.Runtime";
        Descriptor.DisplayName = "Test runtime";
        Descriptor.LanguageId = "test";
        Descriptor.FileExtensions = ".test";
    }

    virtual const Salamatrix::Runtime::RuntimeAdapterDescriptor* WINAPI
    GetDescriptor() const
    {
        return &Descriptor;
    }

    virtual BOOL WINAPI IsAvailable() const { return TRUE; }
    virtual BOOL WINAPI SupportsEntryPoint(const char*) const { return TRUE; }
    virtual BOOL WINAPI Execute(
        const Salamatrix::Runtime::RuntimeExecutionRequest*,
        Salamatrix::Runtime::RuntimeExecutionResult*)
    {
        return FALSE;
    }
};

class TestRuntimeService : public Salamatrix::Runtime::IRuntimeService
{
public:
    Salamatrix::Runtime::IRuntimeAdapter* Registered;

    TestRuntimeService() : Registered(NULL) {}

    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_RUNTIME_VERSION_1_0;
    }

    virtual BOOL WINAPI RegisterAdapter(
        Salamatrix::Runtime::IRuntimeAdapter* adapter)
    {
        if (Registered != NULL || adapter == NULL)
            return FALSE;
        Registered = adapter;
        return TRUE;
    }

    virtual BOOL WINAPI UnregisterAdapter(
        Salamatrix::Runtime::IRuntimeAdapter* adapter)
    {
        if (Registered != adapter)
            return FALSE;
        Registered = NULL;
        return TRUE;
    }

    virtual int WINAPI GetAdapterCount() const
    {
        return Registered == NULL ? 0 : 1;
    }

    virtual Salamatrix::Runtime::IRuntimeAdapter* WINAPI GetAdapter(int index) const
    {
        return index == 0 ? Registered : NULL;
    }

    virtual Salamatrix::Runtime::IRuntimeAdapter* WINAPI FindAdapter(
        const char*, DWORD) const
    {
        return Registered;
    }

    virtual Salamatrix::Runtime::IRuntimeAdapter* WINAPI FindAdapterForEntryPoint(
        const char*) const
    {
        return Registered;
    }
};

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        ++Failures;
    }
}

void TestRoundTrip()
{
    std::string line;
    Check(
        Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageCall,
            42,
            "{\"method\":\"host.call\",\"params\":{}}",
            &line) != FALSE,
        "encode call frame");
    Salamatrix::Runtime::Protocol::LineCodec codec;
    Salamatrix::Runtime::Protocol::Frame frame;
    BOOL complete = FALSE;
    Check(codec.Append(line.c_str(), 3, &frame, &complete) != FALSE &&
              complete == FALSE,
          "partial frame stays buffered");
    Check(codec.Append(line.c_str() + 3, line.size() - 3, &frame, &complete) != FALSE &&
              complete != FALSE,
          "complete frame parses");
    Check(frame.Type == Salamatrix::Runtime::Protocol::MessageCall &&
              frame.Id == 42 &&
              frame.PayloadJson == "{\"method\":\"host.call\",\"params\":{}}",
          "round-trip frame values");
    Check(codec.PendingBytes() == 0, "frame buffer is empty");
}

void TestValidationAndLimits()
{
    std::string line;
    Check(Salamatrix::Runtime::Protocol::LineCodec::Encode(
              Salamatrix::Runtime::Protocol::MessageCall,
              1,
              "{\"bad\":\"line\n\"}",
              &line) == FALSE,
          "reject embedded newline");

    Salamatrix::Runtime::Protocol::LineCodec codec;
    Salamatrix::Runtime::Protocol::Frame frame;
    BOOL complete = FALSE;
    const char malformed[] = "SMX1\tcall\tbad\t{}\n";
    Check(codec.Append(malformed, sizeof(malformed) - 1, &frame, &complete) == FALSE &&
              complete != FALSE,
          "reject malformed request id");

    std::string oversized(Salamatrix::Runtime::Protocol::MaxFrameBytes + 1, 'x');
    Check(codec.Append(oversized.c_str(), oversized.size(), &frame, &complete) == FALSE &&
              complete != FALSE,
          "reject oversized frame");
}

void TestJsonMemberExtraction()
{
    std::string value;
    Check(
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            "{\"params\":{\"ignored\":1},\"method\":\"salamander.storage.get\",\"key\":\"last\\\"Path\"}",
            "method",
            &value) != FALSE && value == "salamander.storage.get",
        "extract method from nested JSON object");
    Check(
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            "{\"params\":{\"ignored\":1},\"method\":\"salamander.storage.get\",\"key\":\"last\\\"Path\"}",
            "key",
            &value) != FALSE && value == "last\"Path",
        "decode JSON string escapes");
    Check(
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            "{\"method\":42}", "method", &value) == FALSE,
        "reject non-string member");
    Check(
        Salamatrix::Runtime::Protocol::Json::FindRawMember(
            "{\"context\":{\"capabilities\":[\"rename\"]},\"prompt\":\"x\"}",
            "context", &value) != FALSE &&
            value == "{\"capabilities\":[\"rename\"]}",
        "extract nested raw context member");
    int coordinate = 0;
    Check(
        Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
            "{\"x\":-12,\"width\":240}", "x", &coordinate) != FALSE &&
            coordinate == -12,
        "extract signed integer member");
    Check(
        Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
            "{\"width\":2147483648}", "width", &coordinate) == FALSE,
        "reject overflowing integer member");
}

void TestRuntimeProviderRegistration()
{
    TestRuntimeService service;
    TestRuntimeAdapter adapter;
    Salamatrix::Runtime::RuntimeProviderRegistration registration;
    Check(registration.Register(&service, &adapter) != FALSE,
          "runtime provider registration succeeds");
    Check(registration.IsRegistered() != FALSE && service.Registered == &adapter,
          "runtime provider retains exact broker and adapter");
    Check(registration.Register(&service, &adapter) == FALSE,
          "runtime provider rejects duplicate registration");
    registration.Unregister();
    Check(registration.IsRegistered() == FALSE && service.Registered == NULL,
          "runtime provider unregisters during release");
}

void TestOwnedServiceRegistry()
{
    Salamatrix::Runtime::ServiceRegistry registry;
    int service = 0;
    int provider = 0;
    int consumer = 0;
    int wrongOwner = 0;
    Check(registry.RegisterServiceOwned(
              "test.owned", 1, &service, "Owned test service", &provider) != FALSE,
          "owned service registration succeeds");
    Check(registry.AcquireService("test.owned", &service, &consumer) != FALSE,
          "consumer can acquire an active owned service");
    Check(registry.ReleaseService("test.owned", &service, &wrongOwner) == FALSE,
          "service lease cannot be released by a different consumer");
    Check(registry.UnregisterServiceOwned("test.owned", &service, &provider) == FALSE,
          "provider cannot unregister while a consumer lease is active");
    Check(registry.ReleaseService("test.owned", &service, &consumer) != FALSE,
          "consumer releases its service lease");
    Check(registry.UnregisterServiceOwned("test.owned", &service, &wrongOwner) == FALSE,
          "different provider cannot unregister an owned service");
    Check(registry.UnregisterServiceOwned("test.owned", &service, &provider) != FALSE,
          "provider unregisters after all leases are released");
    Check(registry.GetCount() == 0, "owned service registry removes released service");
}

void TestAssistantService()
{
    Salamatrix::AI::AssistantService service;
    Check(service.SetContractVersion("Salamatrix.UI", 0x00010000) != FALSE &&
              service.SetContractVersion("Salamatrix.Commands", SALAMATRIX_COMMANDS_VERSION_1_0) != FALSE,
          "register native contract versions for assistant schema");
    FailingAssistantProvider failing;
    TestAssistantProvider provider;
    Salamatrix::AI::AssistantRequest request;
    Salamatrix::AI::AssistantResponse response;
    Check(service.RegisterProvider(&failing) != FALSE,
          "register failing assistant provider");
    Check(service.RegisterProvider(&provider) != FALSE,
          "register assistant provider");
    Check(service.RegisterProvider(&provider) == FALSE,
          "reject duplicate assistant provider");
    Check(service.GetProviderCount() == 2 &&
              service.GetProvider(0) == &failing &&
              service.GetProvider(1) == &provider &&
              service.GetProvider(2) == NULL,
          "enumerate registered assistant providers for native UI");
    Check(service.Generate(NULL, &request, &response) != FALSE &&
              response.Status == Salamatrix::AI::AssistantStatusSucceeded &&
              response.OutputLength != 0,
          "generate falls back to the next available provider");
    std::string runtime;
    Check(
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            response.ResponseJson, "runtime", &runtime) != FALSE &&
            runtime == "Python.CPython",
        "assistant output carries optional runtime hint");
    Check(Salamatrix::AI::IsSafeToRun(response.Summary) != FALSE,
          "read-only assistant output passes safety gate");
    response.Summary.EffectFlags |= Salamatrix::AI::AssistantEffectNetwork;
    Check(Salamatrix::AI::IsSafeToRun(response.Summary) == FALSE,
          "network assistant output is blocked by safety gate");
    Check(strstr(service.GetApiDescription(), "Salamander.ai") != NULL,
          "assistant API description advertises AI object");
    Check(strstr(service.GetApiDescription(), "contractVersions") != NULL &&
              strstr(service.GetApiDescription(), "Salamatrix.UI") != NULL &&
              strstr(service.GetApiDescription(), "1.0") != NULL,
          "assistant API description includes live native contract versions");
    Check(strstr(service.GetApiDescription(), "runtimeAdapters") != NULL,
          "assistant API description includes runtime adapter inventory");
    std::string contractVersions;
    std::string runtimeAdapters;
    Check(Salamatrix::Runtime::Protocol::Json::FindRawMember(
              service.GetApiDescription(), "contractVersions", &contractVersions) != FALSE &&
              contractVersions.size() >= 2 && contractVersions[0] == '{',
          "assistant schema contract versions form a JSON object");
    Check(Salamatrix::Runtime::Protocol::Json::FindRawMember(
              service.GetApiDescription(), "runtimeAdapters", &runtimeAdapters) != FALSE &&
              runtimeAdapters.size() >= 2 && runtimeAdapters[0] == '[',
          "assistant schema runtime inventory forms a JSON array");
    Check(strstr(service.GetApiDescription(), "optional") != NULL,
          "assistant API description advertises optional runtime output");
    Check(strstr(service.GetApiDescription(), "pickFile") != NULL,
          "assistant API description advertises shared file picker");
    Check(strstr(service.GetApiDescription(), "pickFolder") != NULL,
          "assistant API description advertises shared folder picker");
    Check(strstr(service.GetApiDescription(), "setValidation") != NULL,
          "assistant API description advertises dialog validation");
    Check(strstr(service.GetApiDescription(), "onChange") != NULL,
          "assistant API description advertises dialog change events");
    Check(strstr(service.GetApiDescription(), "addColumn") != NULL,
          "assistant API description advertises ListView columns");
    Check(strstr(service.GetApiDescription(), "setSelectedIndex") != NULL,
          "assistant API description advertises control selection");
    Check(strstr(service.GetApiDescription(), "keepOpen") != NULL &&
              strstr(service.GetApiDescription(), "multiline") != NULL,
          "assistant API description advertises shared dialog options");
    Check(strstr(service.GetApiDescription(), "feedback") != NULL,
          "assistant API description advertises repair feedback");
    Check(strstr(service.GetApiDescription(), "Salamander.runtimes") != NULL &&
              strstr(service.GetApiDescription(), "\"list\"") != NULL,
          "assistant API description advertises runtime discovery");
    const char* uiSlice = service.GetApiDescriptionSlice("ui");
    Check(uiSlice != NULL && strstr(uiSlice, "\"topic\":\"ui\"") != NULL &&
              strstr(uiSlice, "dialog.validation") != NULL,
          "assistant API description exposes a focused UI slice");
    const char* uiOptionsSlice = service.GetApiDescriptionSlice("uiOptions");
    Check(uiOptionsSlice != NULL && strstr(uiOptionsSlice, "keepOpen") != NULL,
          "assistant API description exposes shared control options");
    const char* commandSlice = service.GetApiDescriptionSlice("commands");
    Check(commandSlice != NULL && strstr(commandSlice, "hotKey") != NULL,
          "assistant API description exposes a focused command slice");
    const char* sidesSlice = service.GetApiDescriptionSlice("sides");
    Check(sidesSlice != NULL && strstr(sidesSlice, "lastWriteUtc") != NULL &&
              strstr(sidesSlice, "sizeValid") != NULL,
          "assistant API description exposes item metadata");
    Check(strstr(sidesSlice, "tabs") != NULL &&
              strstr(sidesSlice, "activateTab") != NULL &&
              strstr(sidesSlice, "changePath") != NULL &&
              strstr(sidesSlice, "refresh") != NULL &&
              strstr(sidesSlice, "selectItem") != NULL &&
              strstr(sidesSlice, "selectAll") != NULL &&
              strstr(sidesSlice, "focusItem") != NULL &&
              strstr(sidesSlice, "\"version\":\"1.2\"") != NULL,
          "assistant API description exposes tab and selection operations");
    Check(strstr(service.GetApiDescriptionSlice("unknown"),
                 "\"objects\":{}") != NULL,
          "assistant API description handles unknown slices safely");
}

void TestCommandCatalog()
{
    const Salamatrix::Commands::CommandCatalogEntry* entry =
        Salamatrix::Commands::FindCommandCatalogEntry("delete");
    Check(entry != NULL && entry->SalamanderCommandId == SALCMD_DELETE,
          "command catalog exposes delete");
    entry = Salamatrix::Commands::FindCommandCatalogEntry("calculate_directory_sizes");
    Check(entry != NULL && entry->SalamanderCommandId == SALCMD_CALCDIRSIZES,
          "command catalog exposes directory sizes");
}
} // namespace

int main()
{
    TestRuntimeProviderRegistration();
    TestOwnedServiceRegistry();
    TestRoundTrip();
    TestValidationAndLimits();
    TestJsonMemberExtraction();
    TestAssistantService();
    TestCommandCatalog();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d runtime protocol test(s) failed.\n", Failures);
        return 1;
    }
    std::fprintf(stderr, "All runtime protocol tests passed.\n");
    return 0;
}
