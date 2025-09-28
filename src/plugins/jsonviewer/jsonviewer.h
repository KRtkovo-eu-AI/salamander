// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstring>

#include <windows.h>

#include "spl_view.h"
#include "spl_gen.h"
#include "spl_gui.h"

#include "jsonviewer.rh2"

extern const char* PluginNameEN;
extern HINSTANCE DLLInstance;
extern HINSTANCE HLanguage;

extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;

char* LoadStr(int resID);

BOOL InitViewer();
void ReleaseViewer();
void RegisterViewerWindow(HWND hwnd);
void UnregisterViewerWindow(HWND hwnd);

class CJsonViewerWindow;

class CPluginInterfaceForViewer : public CPluginInterfaceForViewerAbstract
{
public:
    virtual BOOL WINAPI ViewFile(const char* name, int left, int top, int width, int height,
                                 UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                 BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                 int enumFilesSourceUID, int enumFilesCurrentIndex);

    virtual BOOL WINAPI CanViewFile(const char* name);
};

class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual void WINAPI About(HWND parent);

    virtual BOOL WINAPI Release(HWND parent, BOOL force);

    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI Configuration(HWND parent);

    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);

    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData) {}

    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer();
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt() { return NULL; }
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() { return NULL; }
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }

    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent);
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}

    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

struct JsonNode
{
    enum class Type
    {
        Object,
        Array,
        String,
        Number,
        Boolean,
        Null
    };

    std::string Key;
    std::string Value;
    Type NodeType;
    std::vector<JsonNode> Children;
};

class JsonParser
{
public:
    explicit JsonParser(const std::string& text);
    std::unique_ptr<JsonNode> Parse();

private:
    std::unique_ptr<JsonNode> ParseValue(const std::string& key);
    std::unique_ptr<JsonNode> ParseObject(const std::string& key);
    std::unique_ptr<JsonNode> ParseArray(const std::string& key);
    std::string ParseString();
    std::string ParseNumber();
    void SkipWhitespace();
    bool MatchLiteral(const char* literal);
    bool End() const;
    char Peek() const;
    char Get();

    const std::string& Text;
    size_t Position;
};

class CJsonViewerWindow
{
public:
    CJsonViewerWindow();
    ~CJsonViewerWindow();

    bool Create(const char* fileName, int left, int top, int width, int height,
                UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock, BOOL* lockOwner);
    HWND GetHWND() const { return HWnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static ATOM EnsureClass();

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void PopulateTree();
    void PopulateNode(HTREEITEM parent, const JsonNode& node);
    BOOL LoadFromFile(const char* fileName);
    void ShowParseError(const char* message);

    HWND HWnd;
    HWND TreeHandle;
    std::unique_ptr<JsonNode> Root;
    std::string FileName;
};

extern CPluginInterface PluginInterface;
extern CPluginInterfaceForViewer InterfaceForViewer;

