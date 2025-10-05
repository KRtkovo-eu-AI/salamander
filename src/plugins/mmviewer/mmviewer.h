﻿// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/*
MP3, MP2 - MPEG 1 or MPEG 2 audio files, layer I, II or III
OGG - OGG Vorbis Audio (competition to MP3) - Ogg Vorbis is a fully open, non-proprietary, patent-and-royalty-free, general-purpose compressed audio format for mid to high quality (8kHz-48.0kHz, 16+ bit, polyphonic) audio and music at fixed and variable bitrates from 16 to 128 kbps/channel. This places Vorbis in the same competitive class as audio representations such as MPEG-4 (AAC), and similar to, but higher performance than MPEG-1/2 audio layer 3, MPEG-4 audio (TwinVQ), WMA and PAC.
VQF - Yamaha VQF Audio (predecessor of MP3, nowadays basically obsolete)
WAV - Waveform Audio
WMA - Windows Media Audio

Modules of music trackers:
669 - Composer 669 Module
IT - Impulse Tracker Module
MOD - Pro Tracker Module
MTM - Multi Tracker Module
S3M - Scream Tracker Module 3
STM - Scream Tracker Module 2
XM - Extended Module - Fast Tracker II
*/

// [0, 0] - for open viewer windows: the plugin configuration has changed
#define WM_USER_VIEWERCFGCHNG WM_APP + 3246
// [0, 0] - for open viewer windows: the history needs to be pruned
#define WM_USER_CLEARHISTORY WM_APP + 3247
// [0, 0] - for open viewer windows: Salamander regenerated fonts, we have to call SetFont() on the lists
#define WM_USER_SETTINGCHANGE WM_APP + 3248

enum
{
    MENUCMD_HTMLEXPORT = 1,
    MENUCMD_FORCEDWORD = 0x7FFFFFFF
};

#define KEY_DOWN(k) (GetAsyncKeyState(k) & 0x8000)

char* FStr(const char* format, ...);
char* LoadStr(int resID);
bool IsUTF8Text(const char* s);
char* AnsiToUTF8(const char* chars, int len);
char* UTF8ToAnsi(const char* chars, int len);
char* WideToAnsi(const wchar_t* chars, int len);
wchar_t* AnsiToWide(const char* chars, int len);
wchar_t* UTF8ToWide(const char* chars, int len);
void ExecuteFile(const char* fname);
BOOL GetOpenFileName(HWND parent, const char* title, char* filter, char* buffer, const char* ext, BOOL save);

int ExportToHTML(const char* fname, COutput& Output);
int ExportToXML(const char* fname, COutput& Output);

// general Salamander interface - valid from startup until the plugin shuts down
extern CSalamanderGeneralAbstract* SalGeneral;

extern HINSTANCE DLLInstance; // handle to the SPL - language-independent resources
extern HINSTANCE HLanguage;   // handle to the SLG - language-dependent resources

// Configuration variables
extern LOGFONT CfgLogFont;                 // description of the font used for the panel
extern BOOL CfgSavePosition;               // save the window position/place it relative to the main window
extern WINDOWPLACEMENT CfgWindowPlacement; // invalid if CfgSavePosition != TRUE

void OnConfiguration(HWND hParent);

extern MENU_TEMPLATE_ITEM PopupMenuTemplate[];

extern CSalamanderGUIAbstract* SalamanderGUI;

extern const char* MMVIEWER_VERSION_STRING;

extern HFONT HNormalFont;
extern HFONT HBoldFont;
extern int FontHeight;

void MMViewerAbout(HWND parent);

//
// ****************************************************************************
// CPluginInterface
//

class CPluginInterfaceForViewer : public CPluginInterfaceForViewerAbstract
{
public:
    virtual BOOL WINAPI ViewFile(const char* name, int left, int top, int width, int height,
                                 UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                 BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                 int enumFilesSourceUID, int enumFilesCurrentIndex);
    virtual BOOL WINAPI CanViewFile(const char* name) { return TRUE; }
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

    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData) { return; }

    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer();
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt();
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() { return NULL; }
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }

    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent);
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}

    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

class CPluginInterfaceForMenuExt : public CPluginInterfaceForMenuExtAbstract
{
public:
    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask);
    virtual BOOL WINAPI ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent,
                                        int id, DWORD eventMask);
    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id);
    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander) {}
};

//
// ****************************************************************************
// CViewerWindow
//

enum CViewerWindowEnablerEnum
{
    vweAlwaysEnabled, // zero index is reserved
    vweFileOpened,
    vweCount
};

class CViewerWindow : public CWindow
{
public:
    HANDLE Lock; // 'lock' object or NULL (signaled only after we close the file)
    CRendererWindow Renderer;

    HWND HRebar; // holds the MenuBar and ToolBar
    CGUIMenuPopupAbstract* MainMenu;
    CGUIMenuBarAbstract* MenuBar;
    CGUIToolBarAbstract* ToolBar;

    HIMAGELIST HGrayToolBarImageList; // toolbar and menu in gray form (derived from the colored one)
    HIMAGELIST HHotToolBarImageList;  // toolbar and menu in the colored form

    DWORD Enablers[vweCount];

public:
    CViewerWindow(int enumFilesSourceUID, int enumFilesCurrentIndex);

    HANDLE GetLock();
    BOOL IsMenuBarMessage(CONST MSG* lpMsg);

    void UpdateEnablers();

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL InitializeGraphics();
    BOOL ReleaseGraphics();

    BOOL InsertMenuBand();
    BOOL InsertToolBarBand();
    BOOL FillToolBar();
    void LayoutWindows();
};
