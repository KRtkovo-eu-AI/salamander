﻿// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "renderer.h"

extern LPCTSTR CAPTURE;

typedef PVCODE(WINAPI* TPVReadImage2)(LPPVHandle Img, HDC PaintDC, RECT* pDRect, TProgressProc Progress, void* AppSpecific, int ImageIndex);
typedef PVCODE(WINAPI* TPVCloseImage)(LPPVHandle Img);
typedef PVCODE(WINAPI* TPVDrawImage)(LPPVHandle Img, HDC PaintDC, int X, int Y, LPRECT rect);
typedef const char*(WINAPI* TPVGetErrorText)(DWORD ErrorCode);
typedef PVCODE(WINAPI* TPVOpenImageEx)(LPPVHandle* Img, LPPVOpenImageExInfo pOpenExInfo, LPPVImageInfo pImgInfo, int Size);
typedef PVCODE(WINAPI* TPVSetBkHandle)(LPPVHandle Img, COLORREF BkColor);
typedef DWORD(WINAPI* TPVGetDLLVersion)(void);
typedef PVCODE(WINAPI* TPVSetStretchParameters)(LPPVHandle Img, DWORD Width, DWORD Height, DWORD Mode);
typedef PVCODE(WINAPI* TPVLoadFromClipboard)(LPPVHandle* Img, LPPVImageInfo pImgInfo, int Size);
typedef PVCODE(WINAPI* TPVGetImageInfo)(LPPVHandle Img, LPPVImageInfo pImgInfo, int Size, int ImageIndex);
typedef PVCODE(WINAPI* TPVSetParam)(LPPVHandle Img);
typedef PVCODE(WINAPI* TPVGetHandles2)(LPPVHandle Img, LPPVImageHandles* pHandles);
typedef PVCODE(WINAPI* TPVSaveImage)(LPPVHandle Img, const char* OutFName, LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific, int ImageIndex);
typedef PVCODE(WINAPI* TPVChangeImage)(LPPVHandle Img, DWORD Flags);
typedef DWORD(WINAPI* TPVIsOutCombSupported)(int Fmt, int Compr, int Colors, int ColorModel);
typedef PVCODE(WINAPI* TPVReadImageSequence)(LPPVHandle Img, LPPVImageSequence* ppSeq);
typedef PVCODE(WINAPI* TPVCropImage)(LPPVHandle Img, int Left, int Top, int Width, int Height);

// Internal helper entry points provided by the in-process imaging backend
typedef bool (*TPVGetRGBAtCursor)(LPPVHandle Img, DWORD Colors, int x, int y, RGBQUAD* pRGB, int* pIndex);
typedef PVCODE (*TPVCalculateHistogram)(LPPVHandle PVHandle, const LPPVImageInfo pvii, LPDWORD luminosity, LPDWORD red, LPDWORD green, LPDWORD blue, LPDWORD rgb);
typedef PVCODE (*TPVCreateThumbnail)(LPPVHandle hPVImage, LPPVSaveImageInfo pSii, int imageIndex, DWORD imgWidth, DWORD imgHeight,
                                     int thumbWidth, int thumbHeight, CSalamanderThumbnailMakerAbstract* thumbMaker, DWORD thumbFlags, TProgressProc progressProc, void* progressProcArg);
typedef PVCODE (*TPVSimplifyImageSequence)(LPPVHandle hPVImage, HDC dc, int ScreenWidth, int ScreenHeight, LPPVImageSequence& pSeq, const COLORREF& bgColor);

PVCODE CreateThumbnail(LPPVHandle hPVImage, LPPVSaveImageInfo pSii, int imageIndex, DWORD imgWidth, DWORD imgHeight,
                       int thumbWidth, int thumbHeight, CSalamanderThumbnailMakerAbstract* thumbMaker, DWORD thumbFlags, TProgressProc progressProc, void* progressProcArg);

PVCODE SimplifyImageSequence(LPPVHandle hPVImage, HDC dc, int ScreenWidth, int ScreenHeight, LPPVImageSequence& pSeq, const COLORREF& bgColor);

struct CPVW32DLL
{
    TPVReadImage2 PVReadImage2;
    TPVCloseImage PVCloseImage;
    TPVDrawImage PVDrawImage;
    TPVGetErrorText PVGetErrorText;
    TPVOpenImageEx PVOpenImageEx;
    TPVSetBkHandle PVSetBkHandle;
    TPVGetDLLVersion PVGetDLLVersion;
    TPVSetStretchParameters PVSetStretchParameters;
    TPVLoadFromClipboard PVLoadFromClipboard;
    TPVGetImageInfo PVGetImageInfo;
    TPVSetParam PVSetParam;
    TPVGetHandles2 PVGetHandles2;
    TPVSaveImage PVSaveImage;
    TPVChangeImage PVChangeImage;
    TPVIsOutCombSupported PVIsOutCombSupported;
    TPVReadImageSequence PVReadImageSequence;
    TPVCropImage PVCropImage;
    TPVGetRGBAtCursor GetRGBAtCursor;
    TPVCalculateHistogram CalculateHistogram;
    TPVCreateThumbnail CreateThumbnail;
    TPVSimplifyImageSequence SimplifyImageSequence;
    HMODULE Handle;   // handle of the active imaging backend module
    char Version[28]; // initialized together with Handle in DllMain on DLL_PROCESS_ATTACH
};

#define WINDOW_POS_SAME 0
#define WINDOW_POS_LARGER 1
#define WINDOW_POS_ANY 2

#define CAPTURE_SCOPE_DESKTOP 0 // full screen
#define CAPTURE_SCOPE_APPL 1    // foreground application (its topmost visible window)
#define CAPTURE_SCOPE_WINDOW 2  // foreground window
#define CAPTURE_SCOPE_CLIENT 3  // client area of foreground window
#define CAPTURE_SCOPE_VIRTUAL 4 // full virtual screen (for multiple monitors)

#define CAPTURE_TRIGGER_HOTKEY 0 // hot key
#define CAPTURE_TRIGGER_TIMER 1  // timer

#define FILES_HISTORY_SIZE (CMD_RECENTFILES_LAST - CMD_RECENTFILES_FIRST + 1)
#define DIRS_HISTORY_SIZE (CMD_RECENTDIRS_LAST - CMD_RECENRDIRS_FIRST + 1)

#define PV_THUMB_CREATE_WIDTH 160
#define PV_THUMB_CREATE_HEIGHT 120

#define PV_MAX_IMG_SIZE_TO_THUMBNAIL 90 // in megapixels

// Flags use din G.DontShowAnymore
#define DSA_UPDATE_THUMBNAILS 1
#define DSA_SAVE_SUCCESS 2
#define DSA_ALPHA_LOST 4

#ifdef ENABLE_WIA
class CWiaWrap;
#endif // ENABLE_WIA
#ifdef ENABLE_TWAIN32
class CTwain;
#endif // ENABLE_TWAIN32

// number of entries in the CopyTo dialog; if changed, adjust the dialog resource as well
#define COPYTO_LINES 5

typedef struct tagGlobals
{
    eZoomType ZoomType;
    BOOL AutoRotate; // Autorotate EXIF JPEG's
    BOOL PipetteInHex;
    BOOL IgnoreThumbnails;                   // TRUE iff thumbnails are always recreated from full-size images
    DWORD MaxThumbImgSize;                   // Do not thumbnailize images with more than that megapixels
    BOOL PageDnUpScrolls;                    // FALSE: PageDown,PageUp & mouse wheel access next/prev image
    int WindowPos;                           // WINDOW_POS_xxx
    int TotalNCWidth;                        // total width of window borders
    int TotalNCHeight;                       // total height of window borders & title & toolbar & statusbar
    BOOL ToolbarVisible;                     // toolbar is visible
    BOOL StatusbarVisible;                   // statusbar is visible
    DWORD LastCfgPage;                       // start page (sheet) in configuration dialog
    int CaptureScope;                        // CAPTURE_SCOPE_xxx
    int CaptureTrigger;                      // CAPTURE_TRIGGER_xxx
    WORD CaptureHotKey;                      // hot key for CAPTURE_TRIGGER_HOTKEY
    int CaptureTimer;                        // delay for CAPTURE_TRIGGER_TIMER
    BOOL CaptureCursor;                      // include mouse cursor to the captured image
    ATOM CaptureAtomID;                      // ID for hot key
    HACCEL HAccel;                           // Accelerator table for the plugin
    CRITICAL_SECTION CS;                     // for accessing FilesHistory and DirsHistory
    LPTSTR FilesHistory[FILES_HISTORY_SIZE]; // Recent Files
    LPTSTR DirsHistory[DIRS_HISTORY_SIZE];   // Recent Directories
    int LastSaveAsFilterIndexMono;           // OPENFILENAME.nFilterIndex for bilevel save
    int LastSaveAsFilterIndexColor;          // OPENFILENAME.nFilterIndex for color save
    COLORREF rgbPanelBackground;             // background color for transparent thumbnails
    SALCOLOR Colors[vceCount];
    int ExifDlgWidth;
    int ExifDlgHeight;
    BOOL bShowPathInTitle;
    int SelectRatioX, SelectRatioY;
    DWORD DontShowAnymore; // list of flags for messages user has asked us to not show anymore
    // Following 3 items are used when hooking Capture dialog because of tooltips
    HHOOK hHook;
    HWND hTTWnd;
    HWND hHookedDlg;
    struct
    {
        DWORD Flags;
        DWORD JPEGQuality;
        DWORD JPEGSubsampling;
        DWORD TIFFStripSize;
        TCHAR InitDir[MAX_PATH];
        BOOL RememberPath; // Remember path when saving screenshots and clipboard pastes
    } Save;

    // Copy To dialog uses globals:
    LPTSTR CopyToDestinations[COPYTO_LINES]; // allocated paths with destinations for Copy To
    int CopyToLastIndex;                     // which path was chosen most recently

    // Print dialog:
    DEVNAMES* pDevNames; // DEVNAMES struct used by ::PrintDlg()
    SIZE_T DevNamesSize; // size of pDevNames
    DEVMODE* pDevMode;   // DEVMODE struct used by ::PrintDlg()
    SIZE_T DevModeSize;  // size of pDevMode
    BOOL bSelectWhole;   // Should CRenameDialog select extension? Cached from global Salamander config
} SGlobals;

typedef struct tagWriteFuncData
{
    CSalamanderThumbnailMakerAbstract* thumbMaker;
    int bytesperline;
    int Size;
    int InSize;
    unsigned char* Buffer;
    int Pos;
} sWriteFuncData, *psWriteFuncData;

typedef struct tagReadMemFuncData
{
    int Size;
    int Pos;
    unsigned char* Buffer;
} sReadMemFuncData, *psReadMemFuncData;

extern TDirectArray<DWORD> ExifHighlights;
extern BOOL ExifGroupHighlights;
extern class CPluginInterfaceForViewer InterfaceForViewer;

typedef WORD TwoWords[2];
typedef DWORD TwoDWords[2];

//
// ****************************************************************************
// CPluginInterface
//

class CPluginInterfaceForViewer : public CPluginInterfaceForViewerAbstract
{
public:
    virtual BOOL WINAPI ViewFile(LPCTSTR name, int left, int top, int width, int height,
                                 UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                 BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                 int enumFilesSourceUID, int enumFilesCurrentIndex);
    virtual BOOL WINAPI CanViewFile(LPCTSTR name);
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

class CPluginInterfaceForThumbLoader : public CPluginInterfaceForThumbLoaderAbstract
{
public:
    virtual BOOL WINAPI LoadThumbnail(LPCTSTR filename, int thumbWidth, int thumbHeight,
                                      CSalamanderThumbnailMakerAbstract* thumbMaker,
                                      BOOL fastThumbnail);
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
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader();

    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent);
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}

    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

//****************************************************************************
//
// CStatusBar
//

class CStatusBar : public CWindow
{
public:
    HICON HCursor;
    HICON HAnchor;
    HICON HSize;
    HICON HPipette;

    HWND hProgBar;

public:
    CStatusBar();
    ~CStatusBar();
};

//
// ****************************************************************************
// CExtraScanImagesToOpen
//

class CExtraScanImagesToOpen
{
protected:
    CRITICAL_SECTION LockCS;                  // section for access to 'Locked'
    BOOL Locked;                              // TRUE = exclusive access to this object when opening windows is granted
    TDirectArray<HBITMAP> AllExtraScanImages; // all scanned images to be opened in the viewer
    CRITICAL_SECTION AESI_CS;                 // section for access to 'AllExtraScanImages'

public:
    CExtraScanImagesToOpen();
    ~CExtraScanImagesToOpen();

    // returns TRUE if exclusive access to this object when opening windows is granted;
    // returns FALSE if this object is already locked
    BOOL LockImages();
    // release exclusive access to this object when opening windows
    void UnlockImages();

    // add images to 'AllExtraScanImages', it takes ownership of images from 'newImgs'
    void AddImages(TDirectArray<HBITMAP>* newImgs);

    // returns TRUE if there is next image in AllExtraScanImages
    BOOL HaveNextImage();
    // give next image from AllExtraScanImages, returned image is considered to be owned
    // by caller, do not forget to release it
    HBITMAP GiveNextImage();
};

//
// ****************************************************************************
// CViewerWindow
//

enum CViewerWindowEnablerEnum
{
    vweAlwaysEnabled,    // zero index is reserved
    vweFileOpened,       // a file is open
    vweFileOpened2,      // a file is open and it comes from disk (not scan, clipboard...)
    vwePaste,            // a bitmap is in the clipboard
    vwePrevPage,         // a previous page is available
    vweNextPage,         // a next page is available
    vweMorePages,        // more pages exist
    vweImgInfoAvailable, // format supported (not necessarily subformat)
    vweImgExifAvailable, // EXIF is present
    vweNotLoading,       // no file or a file is open
    vweSelSrcFile,       // a file is open from disk (not scan, clipboard...) and we have a source connection (panel/Find), so it can be toggled in the source
    vweNextFile,         // we have a source connection (panel/Find) and another file exists in the source
    vwePrevFile,         // we have a source connection (panel/Find) and a previous file exists in the source
    vweNextSelFile,      // we have a source connection (panel/Find) and another selected file exists in the source
    vwePrevSelFile,      // we have a source connection (panel/Find) and a previous selected file exists in the source
    vweFirstFile,        // we have a source connection (panel/Find)
    vweSelection,        // a selection exists
    vweCount
};

class CViewerWindow : public CWindow
{
public:
    HANDLE Lock;              // 'lock' object or NULL (set to signaled state when the file closes)
    CRendererWindow Renderer; // inner window

    HWND HRebar; // holds the MenuBar and ToolBar
    CGUIMenuPopupAbstract* MainMenu;
    CGUIMenuBarAbstract* MenuBar;
    CGUIToolBarAbstract* ToolBar;
    CStatusBar* StatusBar;
#ifdef ENABLE_WIA
    CWiaWrap* WiaWrap; // WIA interface
#endif                 // ENABLE_WIA
#ifdef ENABLE_TWAIN32
    CTwain* Twain;                          // Twain interface
#endif                                      // ENABLE_TWAIN32
    TDirectArray<HBITMAP>* ExtraScanImages; // all scanned images except the first one which is opened in the viewer
    BOOL FullScreen;
    WINDOWPLACEMENT WindowPlacement;
    BOOL AlwaysOnTop;      // from Open Salamander
    HWND HHistogramWindow; // for notifications only

    HIMAGELIST HGrayToolBarImageList; // toolbar and menu in the gray variant (computed from the colored one)
    HIMAGELIST HHotToolBarImageList;  // toolbar and menu in the colored variant

    DWORD Enablers[vweCount];
    BOOL IsSrcFileSelected; // valid only if Enablers[vweSelSrcFile]==TRUE: TRUE/FALSE means the source file is selected/unselected

public:
    CViewerWindow(int enumFilesSourceUID, int enumFilesCurrentIndex, BOOL alwaysOnTop);
    ~CViewerWindow() { ReleaseExtraScanImages(); }

    HANDLE GetLock();

    BOOL IsMenuBarMessage(CONST MSG* lpMsg);
    void UpdateEnablers();
    void UpdateToolBar();
    void ToggleToolBar(); // show/hide the toolbar
    void ToggleStatusBar();

    BOOL IsFullScreen();     // returns TRUE if the viewer is full-screen
    void ToggleFullScreen(); // enable/disable full-screen mode

#ifdef ENABLE_WIA
    BOOL InitWiaWrap();
    void ReleaseWiaWrap();
#endif // ENABLE_WIA

#ifdef ENABLE_TWAIN32
    // twain
    BOOL InitTwain();
    void ReleaseTwain();
#endif // ENABLE_TWAIN32

    BOOL OpenScannedImage(HBITMAP hBitmap);
    void ReleaseExtraScanImages(BOOL deleteImgs = TRUE);
    void OnSize(void);

    // populate the status bar items
    void SetupStatusBarItems();
    void SetStatusBarTexts(int ID = 0);
    void InitProgressBar();
    void KillProgressBar();
    void SetProgress(int done);

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL InitializeGraphics();
    BOOL ReleaseGraphics();
    BOOL FillToolBar();
    BOOL InsertMenuBand();
    BOOL InsertToolBarBand();
    void LayoutWindows();
    void EnsureNoTopmost();
};

//
// Consts
//

// [0, 0] - for open viewer windows: plugin configuration changed
#define WM_USER_VIEWERCFGCHNG WM_APP + 3246

#define WM_USER_CFGDLGDETACH WM_APP + 3247

// [0, 0] - for open viewer windows: Salamander regenerated fonts, call SetFont() on lists
#define WM_USER_SETTINGCHANGE WM_APP + 3248

// [zoom in hundreds of percent, 0]
#define WM_USER_ZOOM WM_APP + 3249

// Handles SaveAs in PV window
#define WM_USER_SAVEAS_INTERNAL WM_APP + 3250

// asks user if we should open extra windows for extra images received from scanner
#define WM_USER_SCAN_EXTRA_IMAGES WM_APP + 3251

// Scroll step sizes for the window
#define XLine 10
#define YLine 10
// Temporary; flag indicating whether pictures should be centered in the window; will be replaced by configuration (??)
#define CFGCenterImage 1

#define ZOOM_SCALE_FACTOR 100000
#define ZOOM_STEP_FACT 1259
#define ZOOM_MAX 16 // 16 means 1600% (like Photoshop)

#define CAPTURE_TIMER_ID 111  // timer ID starting the screen capture
#define CURSOR_TIMER_ID 112   // timer ID for hiding the cursor
#define BRUSH_TIMER_ID 113    // timer ID for shifting the origin of the selection brush
#define SCROLL_TIMER_ID 114   // timer ID handling image scrolling when the cursor leaves the window
#define IMGSEQ_TIMER_ID 115   // id of timer to display next frame of a an image sequence
#define ENABLERS_TIMER_ID 116 // id of timer to run enablers
#define SAVEAS_TIMER_ID 117   // id of timer to wait for path in active panel
#define CLOSEWND_TIMER_ID 118 // id of timer to close window when pop-up window is closed

//
// Functions
//

char* LoadStr(int resID);
WCHAR* LoadStrW(int resID);
BOOL InitViewer(HWND hParentWnd);
void ReleaseViewer();
BOOL InitEXIF(HWND hParent, BOOL bSilent);
bool ConvertPathToExifEncoding(LPCTSTR path, char* buffer, int bufferSize);

#ifndef RC_INVOKED
#include <vector>
#endif

class CExifAnsiPath
{
public:
    CExifAnsiPath();
    ~CExifAnsiPath();

    bool PrepareFromFile(LPCTSTR sourcePath);
    const char* GetPath() const { return Path; }

private:
    char Path[_MAX_PATH];
#ifdef _UNICODE
    TCHAR TempFile[MAX_PATH];
    bool UsingTempCopy;
#endif
};

#ifndef RC_INVOKED
class CExifFileBuffer
{
public:
    CExifFileBuffer();

    bool LoadFromFile(LPCTSTR sourcePath, size_t maxBytes = 16 * 1024 * 1024);
#ifndef _UNICODE
    bool LoadFromWideFile(const wchar_t* sourcePath, size_t maxBytes = 16 * 1024 * 1024);
#endif
    bool HasExifData() const;
    const unsigned char* GetExifData() const;
    unsigned int GetExifSize() const;

private:
    std::vector<unsigned char> Buffer;
    const unsigned char* ExifData;
    unsigned int ExifSize;
};
#endif
void OnConfiguration(HWND hParent);
BOOL MultipleMonitors(RECT* boundingRect);

// history functions
BOOL AddToHistory(BOOL filesHistory, LPCTSTR buff);

BOOL RemoveFromHistory(BOOL filesHistory, int index);
void FillMenuHistory(CGUIMenuPopupAbstract* popup, int cmdFirst, BOOL filesHistory);

// refreshes G.rgbXXX items
void InitGlobalGUIParameters(void);
void RebuildColors(SALCOLOR* colors);

int ShowOneTimeMessage(HWND HParent, int msg, BOOL* pChecked, int flags = MSGBOXEX_YESNO | MSGBOXEX_SILENT,
                       int dontShowMsg = IDS_DONT_SHOW_AGAIN);

// Thumbs.cpp
void UpdateThumbnails(CSalamanderForOperationsAbstract* Salamander);

//
// Externs
//

extern HINSTANCE DLLInstance; // handle to the SPL - language-independent resources
extern HINSTANCE HLanguage;   // handle to the SLG - language-dependent resources

extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;
extern CPVW32DLL PVW32DLL;

extern HINSTANCE EXIFLibrary;

extern LPCTSTR PLUGIN_NAME_EN;
extern LPCTSTR TIP_WINDOW_CLASSNAME;
extern LPCTSTR CLIPBOARD;

extern BOOL SalamanderRegistered; // TRUE = Salamander is licensed (a valid registration key was found)

extern int PredefinedZooms[];
void TrailZeros(LPTSTR buff);

extern SGlobals G;

extern MENU_TEMPLATE_ITEM PopupMenuTemplate[];

extern CWindowQueue ViewerWindowQueue; // list of all viewer windows
