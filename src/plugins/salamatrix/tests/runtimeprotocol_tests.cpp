// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "../precomp.h"
#include "../salamatrix_ai.h"
#include "../salamatrix_commands.h"
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
            "\"script\":\"pass\"}";
        memcpy(response->ResponseJson, result, sizeof(result));
        response->OutputLength = sizeof(result) - 1;
        response->Status = Salamatrix::AI::AssistantStatusSucceeded;
        return TRUE;
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
}

void TestAssistantService()
{
    Salamatrix::AI::AssistantService service;
    TestAssistantProvider provider;
    Salamatrix::AI::AssistantRequest request;
    Salamatrix::AI::AssistantResponse response;
    Check(service.RegisterProvider(&provider) != FALSE,
          "register assistant provider");
    Check(service.RegisterProvider(&provider) == FALSE,
          "reject duplicate assistant provider");
    Check(service.Generate(NULL, &request, &response) != FALSE &&
              response.Status == Salamatrix::AI::AssistantStatusSucceeded &&
              response.OutputLength != 0,
          "generate through default assistant provider");
    Check(strstr(service.GetApiDescription(), "Salamander.ai") != NULL,
          "assistant API description advertises AI object");
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
