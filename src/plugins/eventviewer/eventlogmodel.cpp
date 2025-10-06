#include "precomp.h"
#include "eventlogmodel.h"

namespace
{
std::string FormatFileTime(ULONGLONG fileTime)
{
    FILETIME ft;
    ft.dwLowDateTime = static_cast<DWORD>(fileTime & 0xFFFFFFFF);
    ft.dwHighDateTime = static_cast<DWORD>(fileTime >> 32);
    return FormatEventTime(ft);
}

std::string UnsignedToString(ULONG value)
{
    return FormatUnsigned(value);
}
}

EventLogReader::EventLogReader()
{
}

EventLogReader::~EventLogReader()
{
    for (auto& entry : PublisherMetadataCache)
    {
        if (entry.second != NULL)
            EvtClose(entry.second);
    }
}

EVT_HANDLE EventLogReader::GetPublisherHandle(const std::wstring& provider)
{
    auto it = PublisherMetadataCache.find(provider);
    if (it != PublisherMetadataCache.end())
        return it->second;

    EVT_HANDLE handle = EvtOpenPublisherMetadata(NULL, provider.c_str(), NULL, MAKELCID(LANG_NEUTRAL, SUBLANG_NEUTRAL), 0);
    if (handle != NULL)
        PublisherMetadataCache.emplace(provider, handle);
    return handle;
}

bool EventLogReader::FormatEventMessage(EVT_HANDLE eventHandle, const std::wstring& provider, std::wstring& message)
{
    EVT_HANDLE metadata = GetPublisherHandle(provider);
    if (metadata == NULL)
        return false;

    DWORD bufferSize = 0;
    DWORD bufferUsed = 0;
    if (!EvtFormatMessage(metadata, eventHandle, 0, 0, NULL, EvtFormatMessageEvent, bufferSize, NULL, &bufferUsed))
    {
        DWORD status = GetLastError();
        if (status != ERROR_INSUFFICIENT_BUFFER)
            return false;

        std::vector<wchar_t> buffer(bufferUsed);
        if (!EvtFormatMessage(metadata, eventHandle, 0, 0, NULL, EvtFormatMessageEvent, bufferUsed, &buffer[0], &bufferUsed))
            return false;
        message.assign(&buffer[0], bufferUsed ? bufferUsed - 1 : 0);
        return true;
    }

    return false;
}

bool EventLogReader::RenderEventXml(EVT_HANDLE eventHandle, std::wstring& xmlText)
{
    DWORD bufferSize = 0;
    DWORD bufferUsed = 0;
    DWORD propertyCount = 0;
    if (!EvtRender(NULL, eventHandle, EvtRenderEventXml, bufferSize, NULL, &bufferUsed, &propertyCount))
    {
        DWORD status = GetLastError();
        if (status != ERROR_INSUFFICIENT_BUFFER)
            return false;

        std::vector<wchar_t> buffer(bufferUsed / sizeof(wchar_t));
        if (!EvtRender(NULL, eventHandle, EvtRenderEventXml, bufferUsed, buffer.data(), &bufferUsed, &propertyCount))
            return false;

        xmlText.assign(buffer.data(), bufferUsed / sizeof(wchar_t) - 1);
        return true;
    }

    return false;
}

bool EventLogReader::Query(const wchar_t* logName, size_t maxRecords, std::vector<EventLogRecord>& records, std::string& errorMessage)
{
    records.clear();

    EVT_HANDLE query = EvtQuery(NULL, logName, NULL, EvtQueryChannelPath | EvtQueryReverseDirection);
    if (query == NULL)
    {
        errorMessage = FormatSystemError(GetLastError());
        return false;
    }

    EVT_SYSTEM_PROPERTY_ID systemProperties[] = {
        EvtSystemLevel,
        EvtSystemTimeCreated,
        EvtSystemProviderName,
        EvtSystemEventID,
        EvtSystemTask,
    };

    EVT_HANDLE renderContext = EvtCreateRenderContext(_countof(systemProperties),
                                                     reinterpret_cast<LPCWSTR*>(systemProperties),
                                                     EvtRenderContextSystem);
    if (renderContext == NULL)
    {
        errorMessage = FormatSystemError(GetLastError());
        EvtClose(query);
        return false;
    }

    const DWORD batchSize = 32;
    EVT_HANDLE events[batchSize];
    DWORD returned = 0;
    std::vector<BYTE> valueBuffer;

    while (records.size() < maxRecords)
    {
        if (!EvtNext(query, batchSize, events, INFINITE, 0, &returned))
        {
            DWORD status = GetLastError();
            if (status == ERROR_NO_MORE_ITEMS)
                break;

            errorMessage = FormatSystemError(status);
            EvtClose(renderContext);
            EvtClose(query);
            return false;
        }

        for (DWORD i = 0; i < returned && records.size() < maxRecords; ++i)
        {
            EVT_HANDLE eventHandle = events[i];

            DWORD bufferUsed = 0;
            DWORD propertyCount = 0;
            if (!EvtRender(renderContext, eventHandle, EvtRenderEventValues, 0, NULL, &bufferUsed, &propertyCount))
            {
                DWORD status = GetLastError();
                if (status != ERROR_INSUFFICIENT_BUFFER)
                {
                    errorMessage = FormatSystemError(status);
                    EvtClose(eventHandle);
                    continue;
                }
            }

            valueBuffer.resize(bufferUsed);
            if (!EvtRender(renderContext, eventHandle, EvtRenderEventValues, bufferUsed, valueBuffer.data(), &bufferUsed, &propertyCount))
            {
                errorMessage = FormatSystemError(GetLastError());
                EvtClose(eventHandle);
                continue;
            }

            PEVT_VARIANT values = reinterpret_cast<PEVT_VARIANT>(valueBuffer.data());
            if (propertyCount < _countof(systemProperties))
            {
                EvtClose(eventHandle);
                continue;
            }

            EventLogRecord record;
            record.Level = FormatEventLevel(values[0].ByteVal);
            record.TimeCreated = FormatFileTime(values[1].FileTimeVal);
            std::wstring providerName = values[2].StringVal != NULL ? values[2].StringVal : L"";
            record.Source = WideToAnsi(providerName);
            record.EventId = UnsignedToString(values[3].UInt16Val);
            record.TaskCategory = UnsignedToString(values[4].UInt16Val);

            std::wstring message;
            if (!providerName.empty() && FormatEventMessage(eventHandle, providerName, message))
            {
                record.Details = WideToAnsi(message);
            }
            else if (RenderEventXml(eventHandle, message))
            {
                record.Details = WideToAnsi(message);
            }
            else
            {
                record.Details = LoadStr(IDS_EVENT_DETAILS_NOT_AVAILABLE);
            }

            records.push_back(record);
            EvtClose(eventHandle);
        }
    }

    EvtClose(renderContext);
    EvtClose(query);
    errorMessage.clear();
    return true;
}

std::string FormatSystemError(DWORD errorCode)
{
    char buffer[1024];
    DWORD chars = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorCode, 0, buffer, _countof(buffer), NULL);
    if (chars == 0)
        return "Error " + FormatUnsigned(errorCode);

    while (chars > 0 && (buffer[chars - 1] == '\r' || buffer[chars - 1] == '\n'))
        --chars;

    return std::string(buffer, chars);
}

std::string FormatEventLevel(BYTE level)
{
    int resID = 0;
    switch (level)
    {
    case 1:
        resID = IDS_LEVEL_CRITICAL;
        break;
    case 2:
        resID = IDS_LEVEL_ERROR;
        break;
    case 3:
        resID = IDS_LEVEL_WARNING;
        break;
    case 4:
        resID = IDS_LEVEL_INFORMATION;
        break;
    case 5:
        resID = IDS_LEVEL_VERBOSE;
        break;
    default:
        resID = IDS_LEVEL_UNKNOWN;
        break;
    }

    return LoadStr(resID);
}

std::string FormatEventTime(const FILETIME& fileTime)
{
    FILETIME localFileTime;
    if (!FileTimeToLocalFileTime(&fileTime, &localFileTime))
        localFileTime = fileTime;

    SYSTEMTIME st = {};
    if (!FileTimeToSystemTime(&localFileTime, &st))
    {
        return "";
    }

    char buffer[64];
    _snprintf_s(buffer, _TRUNCATE, "%04d-%02d-%02d %02d:%02d:%02d",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

std::string FormatUnsigned(ULONG value)
{
    char buffer[32];
    _snprintf_s(buffer, _TRUNCATE, "%lu", value);
    return buffer;
}

std::string WideToAnsi(const std::wstring& text)
{
    if (text.empty())
        return std::string();

    int length = WideCharToMultiByte(CP_ACP, 0, text.c_str(), static_cast<int>(text.length()), NULL, 0, NULL, NULL);
    if (length <= 0)
        return std::string();

    std::string result;
    result.resize(length);
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), static_cast<int>(text.length()), &result[0], length, NULL, NULL);
    return result;
}
