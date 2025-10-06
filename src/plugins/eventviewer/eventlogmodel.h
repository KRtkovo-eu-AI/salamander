#pragma once

struct EventLogRecord
{
    std::string Level;
    std::string TimeCreated;
    std::string Source;
    std::string EventId;
    std::string TaskCategory;
    std::string Details;
};

class EventLogReader
{
public:
    EventLogReader();
    ~EventLogReader();

    bool Query(const wchar_t* logName, size_t maxRecords, std::vector<EventLogRecord>& records, std::string& errorMessage);

private:
    EVT_HANDLE GetPublisherHandle(const std::wstring& provider);
    bool FormatEventMessage(EVT_HANDLE eventHandle, const std::wstring& provider, std::wstring& message);
    bool RenderEventXml(EVT_HANDLE eventHandle, std::wstring& xmlText);

    std::map<std::wstring, EVT_HANDLE> PublisherMetadataCache;
};

std::string FormatSystemError(DWORD errorCode);
std::string FormatEventLevel(BYTE level);
std::string FormatEventTime(const FILETIME& fileTime);
std::string FormatUnsigned(ULONG value);
std::string WideToAnsi(const std::wstring& text);
