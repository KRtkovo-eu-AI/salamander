#pragma once

#include "eventlogmodel.h"

class CEventViewerWindow
{
public:
    CEventViewerWindow();
    ~CEventViewerWindow();

    bool Create(HWND parent);
    bool IsCreated() const { return HWindow != NULL; }
    void Show();
    void Close();

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void InitializeControls();
    void InitializeTree();
    void PopulateList();
    void RefreshLog(const std::wstring& logName);
    void DisplayRecordDetails(int index);
    void UpdateLayout(int width, int height);
    void UpdateStatus(const std::string& text);

    HWND HWindow;
    HWND TreeView;
    HWND ListView;
    HWND DetailsEdit;
    HWND StatusBar;

    std::vector<EventLogRecord> Records;
    EventLogReader Reader;
    std::wstring ActiveLog;
    std::vector<std::unique_ptr<std::wstring>> TreeItemStorage;
};
