// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "../precomp.h"
#include "../salamatrix_runtime_protocol.h"

namespace
{
int Failures = 0;

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
}
} // namespace

int main()
{
    TestRoundTrip();
    TestValidationAndLimits();
    TestJsonMemberExtraction();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d runtime protocol test(s) failed.\n", Failures);
        return 1;
    }
    std::fprintf(stderr, "All runtime protocol tests passed.\n");
    return 0;
}
