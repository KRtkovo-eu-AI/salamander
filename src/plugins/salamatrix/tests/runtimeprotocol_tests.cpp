// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "../precomp.h"
#include "../salamatrix_ai.h"
#include "../salamatrix_commands.h"
#include "../salamatrix_runtime_api.h"
#include "../salamatrix_runtime_frame_queue.h"
#include "../salamatrix_runtime_protocol.h"
#include "../../shared/salamatrix_thread_join.h"

namespace
{
int Failures = 0;

class SlowFrameSession : public Salamatrix::Runtime::IRuntimeSession
{
public:
    HANDLE Written;
    std::string Frame;

    SlowFrameSession()
        : Written(CreateEvent(NULL, TRUE, FALSE, NULL))
    {
    }

    virtual ~SlowFrameSession()
    {
        CloseHandle(Written);
    }

    virtual BOOL WINAPI IsAlive() const { return TRUE; }
    virtual BOOL WINAPI SendFrame(const char* bytes, DWORD count)
    {
        Sleep(100);
        Frame.assign(bytes, bytes + count);
        SetEvent(Written);
        return TRUE;
    }
    virtual BOOL WINAPI ReceiveFrame(char*, DWORD, DWORD, DWORD*)
    {
        return FALSE;
    }
    virtual BOOL WINAPI Pump(DWORD) { return FALSE; }
    virtual void WINAPI Stop() {}
    virtual void WINAPI Release() {}
};

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
            "\"capabilities\":[],\"estimatedEffects\":{"
            "\"readSelection\":false,\"readMetadata\":false,"
            "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
            "\"modifyContents\":false,\"executeExternal\":false,\"network\":false},"
            "\"script\":\"pass\",\"runtime\":\"Python.CPython\","
            "\"canImplement\":true,\"missingCapabilities\":[]}";
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
    LONGLONG largeValue = 0;
    Check(
        Salamatrix::Runtime::Protocol::Json::FindInteger64Member(
            "{\"value\":9223372036854770000}", "value", &largeValue) != FALSE &&
            largeValue == 9223372036854770000LL,
        "extract signed 64-bit integer member");
    Check(
        Salamatrix::Runtime::Protocol::Json::FindInteger64Member(
            "{\"value\":9223372036854775808}", "value", &largeValue) == FALSE,
        "reject overflowing 64-bit integer member");
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
    Check(service.SetContractSchema(
              "Salamatrix.Commands",
              "{\"methods\":[\"execute\"]}") != FALSE,
          "register native contract schema fragment for assistant");
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
    Salamatrix::AI::AssistantValidationResult validation;
    Check(service.Validate(&request, &response, &validation) != FALSE &&
              validation.Valid != FALSE,
          "assistant validates a contract-compliant response");
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
    static const char prettyEffectsJson[] =
        "{\n\"title\":\"Sidecar\",\"description\":\"write a file\","
        "\"capabilities\":[\"panels.read\"],\"estimatedEffects\":{\n"
        "\"readSelection\": true,\"readMetadata\": false,"
        "\"renameFiles\": false,\"moveFiles\": false,\"deleteFiles\": false,"
        "\"modifyContents\": true,\"executeExternal\": false,\"network\": false\n  },"
        "\"script\":\"await writeFile(item.path + '.md5', 'x');\","
        "\"runtime\":\"JavaScript.Node\",\"canImplement\":true,"
        "\"missingCapabilities\":[]}";
    memcpy(response.ResponseJson,
           prettyEffectsJson, sizeof(prettyEffectsJson));
    response.OutputLength = sizeof(prettyEffectsJson) - 1;
    Check(service.Validate(NULL, &response, &validation) != FALSE &&
              (response.Summary.EffectFlags &
               Salamatrix::AI::AssistantEffectReadSelection) != 0 &&
              (response.Summary.EffectFlags &
               Salamatrix::AI::AssistantEffectModifyContents) != 0,
          "assistant parses pretty-printed effect booleans structurally");
    Salamatrix::AI::AssistantResponse emptyGap;
    const char emptyGapJson[] =
        "{\"title\":\"Unsupported\",\"description\":\"test\","
        "\"capabilities\":[],\"estimatedEffects\":{"
        "\"readSelection\":false,\"readMetadata\":false,"
        "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
        "\"modifyContents\":false,\"executeExternal\":false,\"network\":false},"
        "\"script\":\"\",\"runtime\":\"JavaScript.Node\","
        "\"canImplement\":false,\"missingCapabilities\":[]}";
    memcpy(emptyGap.ResponseJson, emptyGapJson, sizeof(emptyGapJson));
    emptyGap.OutputLength = sizeof(emptyGapJson) - 1;
    Check(service.Validate(NULL, &emptyGap, &validation) == FALSE &&
              strstr(validation.Message, "at least one host gap") != NULL,
          "assistant rejects canImplement=false without a concrete host gap");
    Salamatrix::AI::AssistantResponse undeclaredExternal;
    const char undeclaredExternalJson[] =
        "{\"title\":\"Unsafe\",\"description\":\"test\","
        "\"capabilities\":[],\"estimatedEffects\":{"
        "\"readSelection\":false,\"readMetadata\":false,"
        "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
        "\"modifyContents\":false,\"executeExternal\":false,\"network\":false},"
        "\"script\":\"import subprocess\",\"runtime\":\"Python.CPython\","
        "\"canImplement\":true,\"missingCapabilities\":[]}";
    memcpy(undeclaredExternal.ResponseJson,
           undeclaredExternalJson, sizeof(undeclaredExternalJson));
    undeclaredExternal.OutputLength = sizeof(undeclaredExternalJson) - 1;
    Check(service.Validate(&request, &undeclaredExternal, &validation) == FALSE &&
              (validation.IssueFlags &
                   Salamatrix::AI::AssistantValidationIssueUnsafeOperation) != 0,
          "assistant rejects undeclared external operations");
    Salamatrix::AI::AssistantResponse unknownCapability;
    const char unknownCapabilityJson[] =
        "{\"title\":\"Invalid\",\"description\":\"test\","
        "\"capabilities\":[\"unknown.capability\"],\"estimatedEffects\":{"
        "\"readSelection\":false,\"readMetadata\":false,"
        "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
        "\"modifyContents\":false,\"executeExternal\":false,\"network\":false},"
        "\"script\":\"pass\",\"runtime\":\"Python.CPython\","
        "\"canImplement\":true,\"missingCapabilities\":[]}";
    memcpy(unknownCapability.ResponseJson,
           unknownCapabilityJson, sizeof(unknownCapabilityJson));
    unknownCapability.OutputLength = sizeof(unknownCapabilityJson) - 1;
    Check(service.Validate(&request, &unknownCapability, &validation) == FALSE &&
              (validation.IssueFlags &
                   Salamatrix::AI::AssistantValidationIssueCapability) != 0,
          "assistant rejects unknown capabilities");
    Salamatrix::AI::AssistantResponse unsupported;
    const char unsupportedJson[] =
        "{\"title\":\"Unsupported\",\"description\":\"The installed API cannot do this.\","
        "\"capabilities\":[],\"estimatedEffects\":{"
        "\"readSelection\":false,\"readMetadata\":false,"
        "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
        "\"modifyContents\":false,\"executeExternal\":false,\"network\":false},"
        "\"script\":\"\","
        "\"canImplement\":false,\"missingCapabilities\":[\"panel columns\",\"thumbnails\"]}";
    memcpy(unsupported.ResponseJson, unsupportedJson, sizeof(unsupportedJson));
    unsupported.OutputLength = sizeof(unsupportedJson) - 1;
    Check(service.Validate(&request, &unsupported, &validation) != FALSE &&
              validation.Valid != FALSE &&
              Salamatrix::AI::AssistantCanImplement(unsupported) == FALSE,
          "assistant accepts an explicit unsupported-capability response");
    Salamatrix::AI::AssistantRequest md5Request;
    md5Request.Prompt =
        "Create an adjacent .md5 sidecar file for every selected file.";
    md5Request.RuntimeId = "JavaScript.Node";
    static const char md5UnsupportedJson[] =
        "{\"title\":\"MD5\",\"description\":\"unsupported\","
        "\"capabilities\":[],\"estimatedEffects\":{"
        "\"readSelection\":false,\"readMetadata\":false,"
        "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
        "\"modifyContents\":false,\"executeExternal\":false,\"network\":false},"
        "\"runtime\":\"JavaScript.Node\",\"canImplement\":false,"
        "\"missingCapabilities\":[\"panel.read\"],\"script\":\"\"}";
    memcpy(response.ResponseJson,
           md5UnsupportedJson, sizeof(md5UnsupportedJson));
    response.OutputLength = sizeof(md5UnsupportedJson) - 1;
    Check(service.Validate(
              &md5Request, &response, &validation) == FALSE &&
              (validation.IssueFlags &
               Salamatrix::AI::AssistantValidationIssueCapability) != 0 &&
              strstr(validation.Message, "is implementable") != NULL,
          "assistant rejects a false framework GAP for selected-file MD5");
    static const char md5ValidJson[] =
        "{\"title\":\"MD5 sidecars\",\"description\":\"Writes MD5 sidecars\","
        "\"capabilities\":[\"panels.read\"],\"estimatedEffects\":{"
        "\"readSelection\":true,\"readMetadata\":false,"
        "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
        "\"modifyContents\":true,\"executeExternal\":false,\"network\":false},"
        "\"runtime\":\"JavaScript.Node\",\"canImplement\":true,"
        "\"missingCapabilities\":[],"
        "\"script\":\"import { createHash } from 'node:crypto'; "
        "import { readFile, writeFile } from 'node:fs/promises'; "
        "const context = await Salamander.sides.context('source'); "
        "for (const item of context.selectedItems) { "
        "const digest = createHash('md5').update(await readFile(item.path)).digest('hex'); "
        "await writeFile(item.path + '.md5', digest); }\"}";
    memcpy(response.ResponseJson, md5ValidJson, sizeof(md5ValidJson));
    response.OutputLength = sizeof(md5ValidJson) - 1;
    Check(service.Validate(&md5Request, &response, &validation) != FALSE &&
              validation.Valid != FALSE,
          "assistant accepts a grounded selected-file MD5 sidecar script");
    Salamatrix::AI::AssistantResponse malformedMissing;
    const char malformedMissingJson[] =
        "{\"title\":\"Invalid\",\"description\":\"test\","
        "\"capabilities\":[],\"estimatedEffects\":{"
        "\"readSelection\":false,\"readMetadata\":false,"
        "\"renameFiles\":false,\"moveFiles\":false,\"deleteFiles\":false,"
        "\"modifyContents\":false,\"executeExternal\":false,\"network\":false},"
        "\"script\":\"pass\",\"canImplement\":true,"
        "\"missingCapabilities\":\"panel columns\"}";
    memcpy(malformedMissing.ResponseJson, malformedMissingJson,
           sizeof(malformedMissingJson));
    malformedMissing.OutputLength = sizeof(malformedMissingJson) - 1;
    Check(service.Validate(&request, &malformedMissing, &validation) == FALSE &&
              (validation.IssueFlags &
                   Salamatrix::AI::AssistantValidationIssueCapability) != 0,
          "assistant rejects malformed missing capabilities");
    Check(strstr(service.GetApiDescription(), "Salamander.ai") != NULL,
          "assistant API description advertises AI object");
    Check(strstr(service.GetApiDescription(), "missingCapabilities") != NULL,
          "assistant API description advertises unsupported-capability output");
    Check(strstr(service.GetApiDescription(), "command_id") != NULL,
          "assistant API description advertises invocation command context");
    Check(strstr(service.GetApiDescription(), "contractVersions") != NULL &&
              strstr(service.GetApiDescription(), "Salamatrix.UI") != NULL &&
              strstr(service.GetApiDescription(), "1.0") != NULL,
          "assistant API description includes live native contract versions");
    Check(strstr(service.GetApiDescription(), "runtimeAdapters") != NULL,
          "assistant API description includes runtime adapter inventory");
    const std::string czechMoveApi =
        Salamatrix::AI::BuildRelevantApiDescription(
            &service,
            "Ozna\xc4\x8d" "en\xc3\xa9 soubory p\xc5\x99"
            "esu\xc5\x88 do prot\xc4\x9b" "j\xc5\xa1"
            "\xc3\xad" "ho panelu.");
    Check(czechMoveApi.find("\"fileOperations\"") != std::string::npos &&
              czechMoveApi.find("\"sides\"") != std::string::npos &&
              czechMoveApi.find("\"all\"") == std::string::npos,
          "Czech move request receives bounded sides and file-operation API slices");
    const std::string extensionApi =
        Salamatrix::AI::BuildRelevantApiDescription(
            &service,
            "Create a Lua extension with a toolbar command and native dialog.");
    Check(extensionApi.find("\"extensions\"") != std::string::npos &&
              extensionApi.find("\"execution\"") != std::string::npos &&
              extensionApi.find("\"commands\"") != std::string::npos &&
              extensionApi.find("\"runtimes\"") != std::string::npos &&
              extensionApi.find("\"ui\"") != std::string::npos &&
              extensionApi.find("generatedPackage") != std::string::npos,
          "extension requests receive manifest, invocation, runtime, command, and UI slices");
    const std::string fullFrameworkApi =
        Salamatrix::AI::BuildRelevantApiDescription(
            &service, "Use the whole framework API.");
    Check(fullFrameworkApi.find("\"clipboard\"") != std::string::npos &&
              fullFrameworkApi.find("\"application\"") != std::string::npos &&
              fullFrameworkApi.find("\"events\"") != std::string::npos &&
              fullFrameworkApi.find("\"ai\"") != std::string::npos,
          "explicit full-framework requests receive every secondary API slice");
    std::string fullFrameworkSlices;
    Check(Salamatrix::Runtime::Protocol::Json::FindRawMember(
              fullFrameworkApi.c_str(), "slices", &fullFrameworkSlices) != FALSE &&
              fullFrameworkSlices.size() >= 2 && fullFrameworkSlices[0] == '{',
          "full-framework API projection is valid nested JSON");
    std::string contractVersions;
    std::string runtimeAdapters;
    std::string nativeContracts;
    Check(Salamatrix::Runtime::Protocol::Json::FindRawMember(
              service.GetApiDescription(), "contractVersions", &contractVersions) != FALSE &&
              contractVersions.size() >= 2 && contractVersions[0] == '{',
          "assistant schema contract versions form a JSON object");
    Check(Salamatrix::Runtime::Protocol::Json::FindRawMember(
              service.GetApiDescription(), "runtimeAdapters", &runtimeAdapters) != FALSE &&
              runtimeAdapters.size() >= 2 && runtimeAdapters[0] == '[',
          "assistant schema runtime inventory forms a JSON array");
    Check(Salamatrix::Runtime::Protocol::Json::FindRawMember(
              service.GetApiDescription(), "nativeContracts", &nativeContracts) != FALSE &&
              nativeContracts.find("Salamatrix.Commands") != std::string::npos &&
              nativeContracts.find("execute") != std::string::npos,
          "assistant schema includes contract-owned method fragments");
    Check(strstr(service.GetApiDescription(), "optional") != NULL,
          "assistant API description advertises optional runtime output");
    Check(strstr(service.GetApiDescription(), "pickFile") != NULL,
          "assistant API description advertises shared file picker");
    Check(strstr(service.GetApiDescription(), "pickFolder") != NULL,
          "assistant API description advertises shared folder picker");
    Check(strstr(service.GetApiDescription(), "folderpicker") != NULL,
          "assistant API description advertises dialog folder picker control");
    Check(strstr(service.GetApiDescription(), "filepicker") != NULL,
          "assistant API description advertises editable file picker control");
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
              strstr(uiSlice, "dialog.validation") != NULL &&
              strstr(uiSlice, "statictext") != NULL &&
              strstr(uiSlice, "toolbarheader") != NULL &&
              strstr(uiSlice, "styleFlags") != NULL &&
              strstr(uiSlice, "buttonMask") != NULL,
          "assistant API description exposes a focused UI slice");
    const char* uiOptionsSlice = service.GetApiDescriptionSlice("uiOptions");
    Check(uiOptionsSlice != NULL && strstr(uiOptionsSlice, "keepOpen") != NULL &&
              strstr(uiOptionsSlice, "filter") != NULL &&
              strstr(uiOptionsSlice, "save") != NULL,
          "assistant API description exposes shared control options");
    const char* commandSlice = service.GetApiDescriptionSlice("commands");
    Check(commandSlice != NULL && strstr(commandSlice, "hotKey") != NULL &&
              strstr(commandSlice, "setState") != NULL &&
              strstr(commandSlice, "visible") != NULL,
          "assistant API description exposes a focused command slice");
    const char* executionSlice = service.GetApiDescriptionSlice("execution");
    Check(executionSlice != NULL && strstr(executionSlice, "command_id") != NULL &&
              strstr(executionSlice, "commandId") != NULL &&
              strstr(executionSlice, "CommandId") != NULL,
          "assistant API description exposes invocation command context");
    const char* extensionSlice = service.GetApiDescriptionSlice("extensions");
    Check(extensionSlice != NULL && strstr(extensionSlice, "schemaVersion") != NULL &&
              strstr(extensionSlice, "extension.json") != NULL &&
              strstr(extensionSlice, "capabilityValues") != NULL,
          "assistant API description exposes extension package authoring");
    const char* applicationSlice = service.GetApiDescriptionSlice("application");
    Check(applicationSlice != NULL && strstr(applicationSlice, "language") != NULL &&
              strstr(applicationSlice, "appearance") != NULL &&
              strstr(applicationSlice, "Windows Dark Mode") != NULL,
          "assistant API description exposes language and appearance state");
    Check(strstr(service.GetApiDescriptionSlice("runtimes"), "Lua") != NULL,
          "assistant runtime slice includes Lua");
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
              strstr(sidesSlice, "createTab") != NULL &&
              strstr(sidesSlice, "closeTab") != NULL &&
              strstr(sidesSlice, "reorderTab") != NULL &&
              strstr(sidesSlice, "moveTab") != NULL &&
              strstr(sidesSlice, "setDetached") != NULL &&
              strstr(sidesSlice, "\"version\":\"1.3\"") != NULL,
          "assistant API description exposes tab mutation operations");
    Check(strstr(service.GetApiDescription(), "\"filter\"") != NULL &&
              strstr(service.GetApiDescription(), "\"save\"") != NULL,
          "assistant API description schema exposes filter/save names");
    Check(strstr(service.GetApiDescription(), "\"tabCreated\"") != NULL &&
              strstr(service.GetApiDescription(), "\"tabClosed\"") != NULL &&
              strstr(service.GetApiDescription(), "\"tabReordered\"") != NULL &&
              strstr(service.GetApiDescription(), "\"windowDetached\"") != NULL &&
              strstr(service.GetApiDescription(), "\"windowAttached\"") != NULL,
          "assistant API description schema exposes tab lifecycle events");
    const char* eventSlice = service.GetApiDescriptionSlice("events");
    Check(eventSlice != NULL && strstr(eventSlice, "\"tabCreated\"") != NULL &&
              strstr(eventSlice, "\"windowAttached\"") != NULL,
          "assistant API description exposes lifecycle events slice");
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

void TestRuntimeFrameQueue()
{
    SlowFrameSession session;
    Salamatrix::Runtime::RuntimeFrameQueue queue;
    Check(queue.Start(&session), "runtime frame queue starts");
    const ULONGLONG started = GetTickCount64();
    Check(queue.Queue("event\n", 6), "runtime frame queue accepts event");
    const ULONGLONG elapsed = GetTickCount64() - started;
    Check(elapsed < 50, "runtime frame queue producer is non-blocking");
    Check(WaitForSingleObject(session.Written, 1000) == WAIT_OBJECT_0 &&
              session.Frame == "event\n",
          "runtime frame queue writes the complete frame asynchronously");
    queue.Shutdown();
}

const UINT TestSentMessage = WM_APP + 197;

LRESULT CALLBACK ThreadJoinWindowProc(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == TestSentMessage)
        return 197;
    return DefWindowProc(window, message, wParam, lParam);
}

struct ThreadJoinContext
{
    HWND Window;
    LRESULT Result;

    ThreadJoinContext() : Window(NULL), Result(0) {}
};

DWORD WINAPI SendSynchronousTestMessage(void* context)
{
    ThreadJoinContext* call = static_cast<ThreadJoinContext*>(context);
    if (call == NULL)
        return 1;
    call->Result = SendMessage(call->Window, TestSentMessage, 0, 0);
    return 0;
}

void TestThreadJoinDispatchesSynchronousMessages()
{
    const wchar_t className[] = L"SalamatrixThreadJoinTest";
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = ThreadJoinWindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.lpszClassName = className;
    Check(RegisterClassW(&windowClass) != 0,
          "register thread-join test window");
    HWND window = CreateWindowExW(
        0, className, L"", 0, 0, 0, 0, 0, HWND_MESSAGE,
        NULL, windowClass.hInstance, NULL);
    Check(window != NULL, "create thread-join test window");
    if (window != NULL)
    {
        ThreadJoinContext context;
        context.Window = window;
        HANDLE thread = CreateThread(
            NULL, 0, SendSynchronousTestMessage, &context, 0, NULL);
        Check(thread != NULL, "start synchronous sender thread");
        if (thread != NULL)
        {
            Check(
                Salamatrix::Runtime::WaitForThreadWithSentMessageDispatch(
                    thread, window) != FALSE,
                "join dispatches synchronous sent message without timeout");
            Check(context.Result == 197,
                  "synchronous sender receives main-thread callback result");
            CloseHandle(thread);
        }
        DestroyWindow(window);
    }
    UnregisterClassW(className, windowClass.hInstance);
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
    TestRuntimeFrameQueue();
    TestThreadJoinDispatchesSynchronousMessages();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d runtime protocol test(s) failed.\n", Failures);
        return 1;
    }
    std::fprintf(stderr, "All runtime protocol tests passed.\n");
    return 0;
}
